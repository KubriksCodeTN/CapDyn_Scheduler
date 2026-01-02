#include "us_delay.h"
#include "msp430fr5994.h"

void us_delay(uint32_t us) {
    TA0CCTL0 = CCIE;
    TA0CCR0 = (us / 100) + 1; // 10khz
    TA0CTL = TASSEL__ACLK | MC__UP;
    __bis_SR_register(LPM0_bits | GIE);
}

/**
 * @brief timer interrupt to stop timer after the delay has passed
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void) {
    TA0CTL = MC__STOP;
    __bic_SR_register_on_exit(LPM0_bits);
}
