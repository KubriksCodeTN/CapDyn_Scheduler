#ifndef __I2C_H__
#define __I2C_H__

#include "intrinsics.h"
#include "i2c_pins.h"

#include <stdint.h>

#warning "This file assumes SMCK at 6MHz!!!"

/* General I2C State Machine */
enum I2C_ModeEnum{
    IDLE_MODE,
    NACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITHC_TO_TX_MODE,
    TIMEOUT_MODE,
};
typedef enum I2C_ModeEnum I2C_mode;
typedef uint8_t err_t;

/**
 * @brief inits I2C with the default parameters:
 *      - clock: 6000000
 *      - clock_src: SMCLK
 *      - data rate 100Kbps
 *      - no auto stop generated
 *
 * @note this implementation uses the driverlib offered by TI
 */
void init_I2C();

/**
 * @brief read using the I2C protocol, this function is used for sensor that require to first
 * write the address of the register to read (for more info see the protocols of the sensors)
 *
 * @param dev_addr target device's address
 * @param reg_addr address of the register to read
 * @param [out] data output buffer (if too small it's undefined behaviour)
 * @param count expected number of bytes to read
 */
err_t I2C_master_read_with_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t count);

/**
 * @brief read using the I2C protocol, this function is used for sensor that do not require to first
 * write the address of the value to read (for more info see the protocols of the sensors)
 *
 * @param dev_addr target device's address
 * @param [out] data output buffer (if too small it's undefined behaviour)
 * @param count expected number of bytes to read
 */
err_t I2C_master_read(uint8_t dev_addr, uint8_t* data, uint8_t count);

/**
 * @brief write using the I2C protocol, this function is used for sensor that require to first
 * write the address of the register to write (for more info see the protocols of the sensors)
 *
 * @param dev_addr target device's address
 * @param reg_addr address of the register to read
 * @param data input buffer (if too small it's undefined behaviour)
 * @param count expected number of bytes to write
 */
err_t I2C_master_write_with_reg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* data, uint8_t count);

/**
 * @brief write using the I2C protocol, this function is used for sensor that do not require to first
 * write the address of the value to write (for more info see the protocols of the sensors)
 *
 * @param dev_addr target device's address
 * @param data input buffer (if too small it's undefined behaviour)
 * @param count expected number of bytes to write
 */
err_t I2C_master_write(uint8_t dev_addr, const uint8_t* data, uint8_t count);

#endif
