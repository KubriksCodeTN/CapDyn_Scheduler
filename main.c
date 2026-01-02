#include "intrinsics.h"
//#include "msp430fr5994.h"
#include <msp430.h> 
#include <scheduler_pins.h>
#include <stdint.h>
#include <stdbool.h>

#include "spi_master.h"
#include "i2c_master.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"
#include "tsl2591.h"
#include "hm0360.h"
#include "sx126x.h"
#include "us_delay.h"
#include "scheduler.h"


uint8_t err0;
tsl2591_t dev;

const uint32_t IMG_SZ = 164 * 124;
#pragma PERSISTENT(fram_v);
uint8_t fram_v[IMG_SZ] = {0};

/**
 * @brief inits the clock
 *
 * @note be careful not to change assumed values without changing the code that assumes said values
 */
void initClock(void) {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_2;

    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz
    CSCTL4 &= ~VLOOFF; // turn on VLOCLK to be used by ACLK (10khz)

    // Set SMCLK = MCLK = DCO, ACLK = VLOCLK
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Per Device Errata set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_6 | DCORSEL;           // Set DCO to 24MHz

    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);
    CSCTL3 = DIVA__1 | DIVS__4 | DIVM__1;   // Set dividers to 1 for 24MHz operation + SMCK div to 4 for camera clock
    CSCTL0_H = 0;                           // Lock CS registers
}

///<
/// inits the various sensors with the needed parameters
void init_sht4x(void){
    sht4x_init(SHT40_I2C_ADDR_44);
}

void init_tsl2591(void) {
    tsl2591_init_desc(&dev, 100000);
    err0 = tsl2591_init(&dev);
    err0 = tsl2591_set_gain(&dev, TSL2591_GAIN_MEDIUM);
    err0 = tsl2591_set_integration_time(&dev, TSL2591_INTEGRATION_200MS);
}

/**
 * @note be careful about camera data pins as they are arbitrary chosen here
 * here the used ones are P3 except for BIT4 which is in P4. It might be best 
 * to use a whole port if possible (this time it was not)
 */
void init_hm0360(void){
    P4DIR &= ~(PCLK | HREF | VSYNC);
    P5DIR |= TRIGGER;
    P5REN &= ~TRIGGER;
    P5OUT &= ~TRIGGER;

    P3DIR = 0; // IN FOR CAMERA DATA EXCEPT FOR 3.4 SMCK, USING 4.4
    P4DIR &= ~BIT4;

    P3DIR |= SMCLK;
    P3SEL0 = 0;
    P3SEL1 |= SMCLK;

    init_internal_registers();
}

void init_sx1262(){
    // NSS
    P6DIR |= BIT3;
    P6OUT |= BIT3;

    // RST
    P5DIR |= BIT2;
    P5OUT |= BIT2;

    //DIO1 (TX IRQ L-to-H)
    P4DIR &= ~BIT7;
    P4REN |= BIT7;
    P4OUT &= ~BIT7;
    P4IES &= ~BIT7;
    
    // required by datasheet
    PM5CTL0 &= ~LOCKLPM5;
    // remove any pending IRQ on port 4
    P4IFG = 0;

    // enable IRQ on P4.7
    P4IE |= BIT7;

    // BUSY
    P8DIR &= ~BIT0;
    P8REN = 0;
}
///>

/**
 * @brief camera task, get a 1 frame in QQVGA
 *
 * @param [out] p output buffer, if too small it's undefined behaviour
 */
void task_hm0360(void* p){
    // P5OUT |= BIT0;

    hm0360_get_image_164x124((uint8_t*)p);
    __no_operation();

    // P5OUT &= ~BIT0;
}

/**
 * @brief lux task, get lux reading
 *
 * @param garbage unused (NULL)
 */
void task_tsl2591(void* garbage){
    // P5OUT |= BIT0;

    int32_t err0;
    float lux = -1.f;

    err0 = tsl2591_set_power_status(&dev, TSL2591_POWER_ON);
    err0 = tsl2591_set_als_status(&dev, TSL2591_ALS_ON);
    __no_operation();
    us_delay(200000); // wait integration time
    tsl2591_get_lux(&dev, &lux);
    __no_operation();
    err0 = tsl2591_set_power_status(&dev, TSL2591_POWER_OFF);
    err0 = tsl2591_set_als_status(&dev, TSL2591_ALS_OFF);
    us_delay(500000);

    // P5OUT &= ~BIT0;
}

/**
 * @brief temp/humid task, get readings
 *
 * @param garbage unused (NULL)
 */
void task_sht4x(void* garbage){
    // P5OUT |= BIT0;


    int32_t temp, humid, err1;
    __no_operation();

    err1 = sht4x_measure_high_precision(&temp, &humid);
    us_delay(500000);
    __no_operation();

    // P5OUT &= ~BIT0;
}

/**
 * @brief LoRa task, transmits the content of fram_v using the radio
 *
 * @note the radio's configuration includes:
 *      - Spreading Factor: 7
 *      - Bandwidth: 500KHz
 *      - CR: 4/5
 */
void task_sx1262(void* p){
    P5OUT |= BIT0;

    sx126x_chip_status_t st;
    sx126x_errors_mask_t errors;

    uint32_t bytes_tx = 0;
    while(bytes_tx < IMG_SZ){
        P1OUT ^= BIT0;

        sx126x_reset(0);
        while(P8IN & BIT0);

        sx126x_set_dio_irq_params(
            0,
            SX126X_IRQ_TIMEOUT | SX126X_IRQ_TX_DONE,
            SX126X_IRQ_TIMEOUT | SX126X_IRQ_TX_DONE,
            SX126X_IRQ_NONE,
            SX126X_IRQ_NONE
        );
        while(P8IN & BIT0);

        sx126x_set_dio3_as_tcxo_ctrl(0, SX126X_TCXO_CTRL_3_3V, 64000);
        while(P8IN & BIT0);

        sx126x_set_pkt_type(0, SX126X_PKT_TYPE_LORA);
        while(P8IN & BIT0);

        sx126x_set_rf_freq(0, 868000000);
        while(P8IN & BIT0);

        sx126x_pa_cfg_params_t pa_cfg; // table 13-21
        pa_cfg.pa_duty_cycle = 0x02;
        pa_cfg.hp_max = 0x02;
        pa_cfg.device_sel = 0;
        pa_cfg.pa_lut = 0x01;
        sx126x_set_pa_cfg(0, &pa_cfg);
        while(P8IN & BIT0);

        sx126x_set_tx_params(0, 0x0E, SX126X_RAMP_40_US); // based on pa_cfg (table 13-21 note 1)
        while(P8IN & BIT0);

        sx126x_set_buffer_base_address(0, 0, 0); // we don't need RxBuff
        while(P8IN & BIT0);

        uint8_t to_send = 255 < IMG_SZ - bytes_tx ? 255 : IMG_SZ - bytes_tx;
        sx126x_write_buffer(0, 0, fram_v + bytes_tx, to_send);
        while(P8IN & BIT0);
        bytes_tx += to_send;

        sx126x_mod_params_lora_t params;
        params.bw = SX126X_LORA_BW_500;
        params.cr = SX126X_LORA_CR_4_5;
        params.sf = SX126X_LORA_SF7;
        params.ldro = 1;
        sx126x_set_lora_mod_params(0, &params);
        while(P8IN & BIT0);

        sx126x_pkt_params_lora_t pkt_params;
        pkt_params.preamble_len_in_symb = 0x08;
        pkt_params.header_type = SX126X_LORA_PKT_EXPLICIT;
        pkt_params.pld_len_in_bytes = to_send;
        pkt_params.crc_is_on = 1;
        pkt_params.invert_iq_is_on = 0;
        sx126x_set_lora_pkt_params(0, &pkt_params);
        while(P8IN & BIT0);

        sx126x_set_lora_sync_word(0, 0x12);
        while(P8IN & BIT0);

        sx126x_cal(0, SX126X_CAL_ALL);
        while(P8IN & BIT0);
        __no_operation();

        sx126x_set_tx(0, 1000); // 1 s timeout
        while(P8IN & BIT0);

/*
        sx126x_get_status(0, &st);
        while(P8IN & BIT0);
        __no_operation();
*/
        __bis_SR_register(CPUOFF | GIE);


        sx126x_irq_mask_t irq_status;
        sx126x_get_irq_status(0, &irq_status);
        __no_operation();
    }

    sx126x_set_sleep(0, SX126X_SLEEP_CFG_COLD_START);

    P5OUT &= ~BIT0;
}

// ! various pins used, CARE NOT TO FUCK UP THE SCHEDULER OR HIBERNUS
/*
 *      SCHEDULER
 * 1.2 - SHIFT_REG_DATA
 * 1.3 - WKP_LINE
 * 1.4 - SHIFT_REG_CLK
 * 1.5 - SHIFT_REG_LATCH
 * 4.3 - EOT
 *
 *       I2C
 * 7.0 - SDA
 * 7.1 - SCL
 *
 *       HM0360
 * 3.4 - SMCK
 * 4.1 - PCLK
 * 4.2 - HREF
 * 4.3 - VSYNC
 * 5.5 - TRIGGER
 * 3.0 - D0
 * 3.1 - D1
 * 3.2 - D2
 * 3.3 - D3
 * 4.4 - D4 !!!!!
 * 3.5 - D5
 * 3.6 - D6
 * 3.7 - D7
 *
 *       SPI
 * 6.0 - MOSI
 * 6.1 - MISO
 * 6.2 - SCK
 *
 *       LORA
 * 6.3 - NSS
 * 5.2 - RST
 * 4.1 - DIO1
 * 8.0 - BUSY
 * VCC - DIO3
 *
 */


int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    initClock(); // static config

    /*** USER SETUP ***/
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    P1DIR |= BIT0 | BIT1;
    P1OUT &= ~(BIT0 | BIT1);

    P1DIR |= BIT0 | BIT1;
    P1OUT &= ~(BIT0 | BIT1);

    P2OUT = 0;
    P2DIR = 0xFF;

    P3OUT = 0;
    P3DIR = 0xFF;

    P4OUT = 0;
    P4DIR = 0xFF;

    P5OUT = 0;
    P5DIR = 0xFF;

    P6OUT = 0;
    P6DIR = 0xFF;

    P7OUT = 0;
    P7DIR = 0xFF;

    P8OUT = 0;
    P8DIR = 0xFF;

    PJOUT = 0;
    PJDIR = 0xFFFF;

    // static HW configs + sensors which cannot be snapshotted anyway
    init_I2C();
    init_SPI();
    us_delay(20000); // wait for settle
    init_sht4x();
    init_tsl2591();
    init_hm0360();
    init_sx1262();
    __no_operation();
    __bis_SR_register(GIE);

    hibernus_init();

    uint8_t task_id = 0;
    task_t array[4];
    task_create(array + task_id++, &task_sht4x, NULL, 2);
    task_create(array + task_id++, &task_tsl2591, NULL, 3);
    task_create(array + task_id++, &task_hm0360, (void*)fram_v, 4);
    task_create(array + task_id++, &task_sx1262, NULL, 5);

    scheduler_init(array, task_id, 0);
    scheduler_start();
}

#pragma vector=PORT4_VECTOR
__interrupt void port4_isr_handler(void){
    switch(__even_in_range(P4IV, P4IV__P4IFG7))
    {
        case P4IV__NONE:    break;          // Vector  0:  No interrupt
        case P4IV__P4IFG0:  break;          // Vector  2:  P3.0 interrupt flag
        case P4IV__P4IFG1:  break;          // Vector  4:  P3.1 interrupt flag
        case P4IV__P4IFG2:  break;          // Vector  6:  P3.2 interrupt flag
        case P4IV__P4IFG3:  break;          // Vector  8:  P3.3 interrupt flag
        case P4IV__P4IFG4:  break;          // Vector  10:  P3.4 interrupt flag
        case P4IV__P4IFG5:  break;          // Vector  12:  P3.5 interrupt flag
        case P4IV__P4IFG6:  break;          // Vector  14:  P3.6 interrupt flag
        case P4IV__P4IFG7:
            __bic_SR_register_on_exit(CPUOFF);
            break;
        default: break;
    }
}

/*
void adc_task(void* garbage) {
    WDTCTL = WDTPW | WDTHOLD;                      // Stop WDT

    // GPIO Setup
    P1OUT &= ~(BIT0 | BIT1);                       // Clear LED to start
    P1DIR |= BIT0 | BIT1;                          // Set P1.0/LED to output
    P3SEL1 |= BIT0 | BIT1;                         // Configure P3.0 / P3.1 for ADC
    P3SEL0 |= BIT0 | BIT1;                         

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure ADC12
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON | ADC12MSC;      // Sampling time, S&H=16, ADC12 on
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;              // Use sampling timer
    ADC12CTL2 |= ADC12RES_2;                           // 12-bit conversion results
    ADC12MCTL0 |= ADC12INCH_12;
    ADC12MCTL1 |= ADC12INCH_13;                        // A1 ADC input select; Vref=AVCC
    ADC12IER0 |= ADC12IE0 | ADC12IE1;                  // Enable ADC conv complete interrupt

    while (1) {
        __delay_cycles(5000000); // also us_delay(x) is fine
        ADC12CTL0 |= ADC12ENC | ADC12SC;    // Start sampling/conversion

        __bis_SR_register(LPM0_bits | GIE); // LPM0, ADC12_ISR will force exit
        // __no_operation();                   // For debugger
    }
}
*/

/*
// ADC interrupt for end of conversion, blinks a led iff the measurement is high engouh
// double channel so 2 different interrupts are possible
// during measurements led were not turned on as they waste energy, they are here only to 
// debug, remove them if needed
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_B_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow
        case ADC12IV__ADC12HIIFG:  break;   // Vector  6:  ADC12BHI
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0 Interrupt
            if (ADC12MEM0 >= 0x7ff)         // ADC12MEM0 = A1 > 0.5AVcc? <-- measurement 0
                P1OUT |= BIT0;              // P1.0 = 1
            else
                P1OUT &= ~BIT0;             // P1.0 = 0

            // Exit from LPM0 and continue executing main
            __bic_SR_register_on_exit(LPM0_bits);
            break;
        case ADC12IV__ADC12IFG1:
            if (ADC12MEM1 >= 0x7ff)         // ADC12MEM0 = A1 > 0.5AVcc? <-- measurement 1
                P1OUT |= BIT1;              // P1.0 = 1
            else
                P1OUT &= ~BIT1;             // P1.0 = 0
            __bic_SR_register_on_exit(LPM0_bits);
            break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }
}
*/
