#include "i2c_master.h"
//#include "msp430fr5994.h"
#include "driverlib\MSP430F5xx_6xx\driverlib.h"

/* Used to track the state of the software state machine*/
volatile I2C_mode master_mode = IDLE_MODE;

/* The Register Address/Command to use*/
volatile uint8_t transmit_reg_addr;

volatile uint8_t* RX_buffer;
volatile uint8_t RX_byte_ctr = 0;
volatile uint8_t RX_idx = 0;
volatile const uint8_t* TX_buffer;
volatile uint8_t TX_byte_ctr = 0;
volatile uint8_t TX_idx = 0;

void init_I2C() {
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings

    // I2C pins
    P7SEL0 |= SDA | SCL;
    P7SEL1 &= ~(SDA| SCL);


    EUSCI_B_I2C_initMasterParam param = {0};
    param.selectClockSource = EUSCI_B_I2C_CLOCKSOURCE_SMCLK;
    param.i2cClk = 6000000;
    param.dataRate = EUSCI_B_I2C_SET_DATA_RATE_100KBPS;
    param.byteCounterThreshold = 0;
    param.autoSTOPGeneration = EUSCI_B_I2C_NO_AUTO_STOP;
    EUSCI_B_I2C_initMaster(EUSCI_B2_BASE, &param);
}

err_t I2C_master_read_with_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t count) {

    /* Initialize state machine */
    master_mode = TX_REG_ADDRESS_MODE;
    transmit_reg_addr = reg_addr;
    RX_buffer = data;
    RX_byte_ctr = count;
    TX_byte_ctr = 0;
    RX_idx = 0;
    TX_idx = 0;

    EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE,
        dev_addr
    );

    //Set Master in receive mode
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_MODE // even if this is the read function, the first thing I have to do is sending reg_addr
    );

    //Enable I2C Module to start operations
    EUSCI_B_I2C_enable(EUSCI_B2_BASE);

    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    //Enable master Receive interrupt
    EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0
    );

    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B2_BASE));

    EUSCI_B_I2C_masterSendStart(EUSCI_B2_BASE);

    // Enter LPM0 w/ interrupt
    __bis_SR_register(CPUOFF+GIE);

    return RX_byte_ctr;
}

err_t I2C_master_read(uint8_t dev_addr, uint8_t* data, uint8_t count) {

    //Specify slave address
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE,
        dev_addr
    );

    //Set Master in receive mode
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_MODE
    );

//Enable I2C Module to start operations
    EUSCI_B_I2C_enable(EUSCI_B2_BASE);

    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0
    );


    /* Initialize state machine */
    master_mode = RX_DATA_MODE;
    RX_buffer = data;
    RX_byte_ctr = count;
    TX_byte_ctr = 0;
    RX_idx = 0;
    TX_idx = 0;


    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B2_BASE));


    if (RX_byte_ctr == 1) {
        RX_buffer[0] = EUSCI_B_I2C_masterReceiveSingleByte(EUSCI_B2_BASE);
        master_mode = IDLE_MODE;
        return RX_byte_ctr;
    }

    //Enable master Receive interrupt
   EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
       EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
       EUSCI_B_I2C_NAK_INTERRUPT
   );

    // Enter LPM0 w/ interrupt
    EUSCI_B_I2C_masterReceiveStart(EUSCI_B2_BASE);
    __bis_SR_register(CPUOFF+GIE);

    return RX_byte_ctr;
}

err_t I2C_master_write_with_reg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint8_t count) {

    // Initialize state machine
    master_mode = TX_REG_ADDRESS_MODE;
    transmit_reg_addr = reg_addr;
    TX_buffer = data;
    TX_byte_ctr = count;
    RX_byte_ctr = 0;
    RX_idx = 0;
    TX_idx = 0;

    //Specify slave address
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE,
          dev_addr
    );

    //Set Master in receive mode
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_MODE
    );

    //Enable I2C Module to start operations
    EUSCI_B_I2C_enable(EUSCI_B2_BASE);


    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    //Enable master Receive interrupt
    EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_RECEIVE_INTERRUPT0
    );

    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B2_BASE));

    EUSCI_B_I2C_masterSendStart(EUSCI_B2_BASE);

    __bis_SR_register(LPM0_bits + GIE);      // Enter LPM0 w/ interrupts

    return TX_byte_ctr;
}

err_t I2C_master_write(uint8_t dev_addr, const uint8_t* data, uint8_t count) {

    //Specify slave address
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B2_BASE,
          dev_addr
    );

    //Set Master in receive mode
    EUSCI_B_I2C_setMode(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_MODE
    );

    //Enable I2C Module to start operations
    EUSCI_B_I2C_enable(EUSCI_B2_BASE);


    EUSCI_B_I2C_clearInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    //Enable master Receive interrupt
    EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
        EUSCI_B_I2C_TRANSMIT_INTERRUPT0 +
        EUSCI_B_I2C_NAK_INTERRUPT
    );

    EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE,

        EUSCI_B_I2C_RECEIVE_INTERRUPT0
    );

    /* Initialize state machine */
    master_mode = TX_DATA_MODE;
    TX_buffer = data;
    TX_byte_ctr = count - 1;
    RX_byte_ctr = 0;
    RX_idx = 0;
    TX_idx = 0;

    while (EUSCI_B_I2C_SENDING_STOP == EUSCI_B_I2C_masterIsStopSent(EUSCI_B2_BASE));

    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B2_BASE, TX_buffer[TX_idx++]);

    __bis_SR_register(LPM0_bits + GIE);

    return TX_byte_ctr;
}

/**
 * @brief I2C interrupt that handles the I2C state machine and the protocol communication
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B2_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_B2_VECTOR)))
#endif
void USCIB2_ISR(void)
{
    switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG))
    {
        case USCI_NONE:             // No interrupts break;
            break;
        case USCI_I2C_UCALIFG:      // Arbitration lost
            __no_operation();
            break;
        case USCI_I2C_UCNACKIFG:    // NAK received (master only)
            __no_operation();
            P1OUT |= BIT1;
            master_mode = IDLE_MODE;
            __bic_SR_register_on_exit(CPUOFF);
            break;
        case USCI_I2C_UCTXIFG0:     // TXIFG0
            switch(master_mode){
            case TX_REG_ADDRESS_MODE:
                EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B2_BASE, transmit_reg_addr);

                if (RX_byte_ctr)
                    master_mode = SWITCH_TO_RX_MODE;   // Need to start receiving now
                else
                    master_mode = TX_DATA_MODE;        // Continue to transmission with the data in Transmit Buffer
            break;

            case SWITCH_TO_RX_MODE:
                master_mode = RX_DATA_MODE;     // From now on, I want to receive

                EUSCI_B_I2C_setMode(EUSCI_B2_BASE, EUSCI_B_I2C_RECEIVE_MODE);

                EUSCI_B_I2C_enableInterrupt(EUSCI_B2_BASE,
                    EUSCI_B_I2C_RECEIVE_INTERRUPT0 +
                    EUSCI_B_I2C_NAK_INTERRUPT
                );

                EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE,EUSCI_B_I2C_TRANSMIT_INTERRUPT0);

                EUSCI_B_I2C_masterReceiveStart(EUSCI_B2_BASE);

                if (RX_byte_ctr == 1) {
                    // Must send stop since this is the N-1 byte
                    while (EUSCI_B_I2C_SENDING_START == EUSCI_B_I2C_masterIsStartSent(EUSCI_B2_BASE));
                    EUSCI_B_I2C_masterReceiveMultiByteStop(EUSCI_B2_BASE);
                }
            break;

            case TX_DATA_MODE:
                // Check TX byte counter
                if (TX_byte_ctr)
                {
                    EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B2_BASE, TX_buffer[TX_idx++]);
                    // Decrement TX byte counter
                    TX_byte_ctr--;
                }
                else
                {
                    EUSCI_B_I2C_masterSendMultiByteStop(EUSCI_B2_BASE);
                    EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
                    master_mode = IDLE_MODE;
                    // Exit LPM0
                    __bic_SR_register_on_exit(CPUOFF);
                }
                break;
            }
            break;
        default:
            break;
        case USCI_I2C_UCRXIFG0:     // RXIFG0
            // Get RX data
            if (RX_byte_ctr) {
                RX_buffer[RX_idx++] = EUSCI_B_I2C_masterReceiveSingle(EUSCI_B2_BASE);
                --RX_byte_ctr;
            }
            if (RX_byte_ctr == 1) {
                EUSCI_B_I2C_masterReceiveMultiByteStop(EUSCI_B2_BASE);
            }
            else if (RX_byte_ctr == 0) {
                EUSCI_B_I2C_disableInterrupt(EUSCI_B2_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0);
                master_mode = IDLE_MODE;
                __bic_SR_register_on_exit(LPM0_bits | GIE);      // Exit LPM0
            }
            break;
    }
}
