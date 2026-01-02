#include "spi_master.h"
#include "driverlib\MSP430F5xx_6xx\driverlib.h"
#include "us_delay.h"

typedef enum{
    IDLE_MODE,
    CMD_DATA_MODE,
    RX_DATA_MODE,
    TX_DATA_MODE,
} SPI_master_mode_t;

volatile SPI_master_mode_t spi_master_mode;

volatile uint8_t* spi_RX_buffer;
volatile uint8_t spi_RX_byte_ctr;
volatile uint8_t spi_RX_idx;
volatile const uint8_t* cmd_buffer;
volatile uint8_t cmd_byte_ctr;
volatile uint8_t cmd_idx;
volatile const uint8_t* spi_TX_buf;
volatile uint8_t spi_TX_byte_ctr;
volatile uint8_t spi_TX_idx;

void init_SPI(){

    // MOSI MISO SCK (secondary fun)
    P6SEL0 |= MOSI | MISO | SCK;
    P6SEL1 &= ~(MOSI | MISO| SCK);


    //Initialize Master
    EUSCI_A_SPI_initMasterParam param = {0};
    param.selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK;
    param.clockSourceFrequency = 6000000;
    param.desiredSpiClock = 500000;
    param.msbFirst = EUSCI_A_SPI_MSB_FIRST;
    param.clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT;
    param.clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW;
    param.spiMode = EUSCI_A_SPI_3PIN;

    EUSCI_A_SPI_initMaster(EUSCI_A3_BASE, &param);

    // EUSCI_A_SPI_select4PinFunctionality(EUSCI_A3_BASE, EUSCI_A_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE);

}

uint8_t SPI_read(const uint8_t* command, const uint16_t command_length, uint8_t* data, const uint16_t data_length){
    //Enable SPI module
    EUSCI_A_SPI_enable(EUSCI_A3_BASE);

    EUSCI_A_SPI_clearInterrupt(EUSCI_A3_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);
    // Enable USCI_A3 RX interrupt
    EUSCI_A_SPI_enableInterrupt(EUSCI_A3_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);

    //Wait for slave to initialize
    __delay_cycles(100);

    spi_master_mode = CMD_DATA_MODE;
    spi_RX_buffer = data;
    spi_RX_byte_ctr = data_length;
    spi_RX_idx = 0;
    cmd_buffer = command;
    cmd_byte_ctr = command_length;
    cmd_idx = 0;
    spi_TX_byte_ctr = 0;
    spi_TX_idx = 0;
    spi_TX_buf = 0;

    //USCI_A3 TX buffer ready?
    while (!EUSCI_A_SPI_getInterruptStatus(EUSCI_A3_BASE, EUSCI_A_SPI_TRANSMIT_INTERRUPT));

    P6OUT &= ~BIT3; // NSS low
    us_delay(150); // wait for radio to be ready receiving

    //Transmit Data to slave
    cmd_byte_ctr--;
    EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, cmd_buffer[cmd_idx++]);

    // Enter LPM0 w/ interrupt
    __bis_SR_register(CPUOFF+GIE);

    return spi_RX_byte_ctr;
}

uint8_t SPI_write(const uint8_t* command, const uint16_t command_length, const uint8_t* data, const uint16_t data_length){
    //Enable SPI module
    EUSCI_A_SPI_enable(EUSCI_A3_BASE);

    EUSCI_A_SPI_clearInterrupt(EUSCI_A3_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);
    // Enable USCI_A3 RX interrupt
    EUSCI_A_SPI_enableInterrupt(EUSCI_A3_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);

    //Wait for slave to initialize
    __delay_cycles(100);

    spi_master_mode = CMD_DATA_MODE;
    spi_TX_buf = data;
    spi_TX_byte_ctr = data_length;
    spi_TX_idx = 0;
    spi_RX_buffer = 0;
    spi_RX_byte_ctr = 0;
    spi_RX_idx = 0;
    cmd_buffer = command;
    cmd_byte_ctr = command_length;
    cmd_idx = 0;

    //USCI_A3 TX buffer ready?
    while (!EUSCI_A_SPI_getInterruptStatus(EUSCI_A3_BASE, EUSCI_A_SPI_TRANSMIT_INTERRUPT));

    P6OUT &= ~BIT3; // NSS low
    us_delay(150); // wait for radio to be ready receiving

    //Transmit Data to slave
    cmd_byte_ctr--;
    EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, cmd_buffer[cmd_idx++]);

    // Enter LPM0 w/ interrupt
    __bis_SR_register(CPUOFF+GIE);

    return spi_TX_byte_ctr;
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A3_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_A3_VECTOR)))
#endif
void USCI_A3_ISR (void)
{

    uint8_t tmp;
    switch (__even_in_range(UCA3IV, USCI_SPI_UCTXIFG))
    {

    case USCI_SPI_UCRXIFG:
        //USCI_A3 TX buffer ready?
        while (!EUSCI_A_SPI_getInterruptStatus(EUSCI_A3_BASE, EUSCI_A_SPI_TRANSMIT_INTERRUPT));
        switch(spi_master_mode){

        case CMD_DATA_MODE:
            /*
            tmp = EUSCI_A_SPI_receiveData(EUSCI_A3_BASE);
            tmp &= 0b01111110;
            */
            if(cmd_byte_ctr > 0){

                cmd_byte_ctr--;
                EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, cmd_buffer[cmd_idx++]);
                return;
            }


            if(spi_RX_byte_ctr > 0){
                spi_master_mode = RX_DATA_MODE;
                EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, 0); // send NOP
            }
            else if(spi_TX_byte_ctr > 0){
                spi_master_mode = TX_DATA_MODE;
                spi_TX_byte_ctr--;
                EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, spi_TX_buf[spi_TX_idx++]);
            }
            else{
                P6OUT |= BIT3;
                EUSCI_A_SPI_disable(EUSCI_A3_BASE);
                spi_master_mode = IDLE_MODE;
                __bic_SR_register_on_exit(CPUOFF);
            }

        break;

        case TX_DATA_MODE:
            if(!spi_TX_byte_ctr){
                P6OUT |= BIT3;
                EUSCI_A_SPI_disable(EUSCI_A3_BASE);
                spi_master_mode = IDLE_MODE;
                __bic_SR_register_on_exit(CPUOFF);
            }

            else{
                spi_TX_byte_ctr--;
                EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, spi_TX_buf[spi_TX_idx++]);
            }

        break;



        case RX_DATA_MODE:
            if(spi_RX_byte_ctr > 0){
                spi_RX_byte_ctr--;
                tmp = EUSCI_A_SPI_receiveData(EUSCI_A3_BASE);
                __no_operation();
                spi_RX_buffer[spi_RX_idx++] = tmp;

                if(!spi_RX_byte_ctr){
                    P6OUT |= BIT3; //NSS high
                    EUSCI_A_SPI_disable(EUSCI_A3_BASE);
                    spi_master_mode = IDLE_MODE;
                    __bic_SR_register_on_exit(CPUOFF);
                }
                else
                    EUSCI_A_SPI_transmitData(EUSCI_A3_BASE, 0); // send NOP

            }
            break;

        default:
            break;
        }
    default:
        break;
    }
}

