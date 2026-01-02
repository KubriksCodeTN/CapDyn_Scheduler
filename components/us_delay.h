#ifndef __US_DELAY_H__
#define __US_DELAY_H__

#warning "This file assumes ACLK at 10KHz!!!"

#include "intrinsics.h"
#include "msp430fr5994.h"
#include <msp430.h>
#include <stdint.h>

/**
 * @brief timer delay in us
 *
 * @param us the delay in us
 *
 * @note assumes a specific ACLK setup, modify if needed
 */
void us_delay(uint32_t us);

#endif
