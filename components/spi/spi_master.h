#ifndef _SPI_MASTER_H_
#define _SPI_MASTER_H_

#include <stdint.h>
#include "spi_pins.h"

#warning "This file assumes SMCLK at 6MHz"

/**
 * @brief inits SPI with the default parameters:
 *      - clock: 6000000
 *      - clock_src: SMCLK
 *      - data rate 500Kbps
 *
 * @note this implementation uses the driverlib offered by TI
 */
void init_SPI();

/**
 * @brief read using the SPI protocol
 *
 * @param command target command
 * @param command_length command's length in bytes
 * @param [out] data output buffer (if too small it's undefined behaviour)
 * @param data_length expected number of bytes to read
 */
uint8_t SPI_read(const uint8_t* command, const uint16_t command_length, uint8_t* data, const uint16_t data_length);

/**
 * @brief write using the SPI protocol
 *
 * @param command target command
 * @param command_length command's length in bytes
 * @param data additional parameters' buffer
 * @param data_length parameters' length in bytes
 */
uint8_t SPI_write(const uint8_t* command, const uint16_t command_length, const uint8_t* data, const uint16_t data_length);

#endif
