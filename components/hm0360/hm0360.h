/*
 * Copyright 2021 Arduino SA
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * HM0360 driver.
 */
#ifndef __HIMAX_H
#define __HIMAX_H

#include "../i2c/i2c_master.h"
#include "../us_delay.h"
#include "pins.h"
#include <stdbool.h>

#define HM0360_I2C_ADDR 0x24

/**
 * @brief platform dependant writing procedure via I2C
 *
 * @param reg_addr address of the register to write
 * @param reg_data byte to write in the register
 *
 * @note the bool parameter is should always be true with this driver 
 */
int hm0360_regWrite(uint16_t reg_addr, uint8_t reg_data, bool);

/**
 * @brief platform dependant reading procedure via I2C
 *
 * @param reg_addr address of the register to read
 *
 * @note the bool parameter is should always be true with this driver 
 */
uint8_t hm0360_regRead(uint16_t reg_addr, bool);

/**
 * @brief sets the camera in the default state, if a different default is needed change the code 
 * accordingly
 *
 * @note the default config is:
 *      - max prescaler
 *      - gated by line (see datasheet if needed)
 *      - 1 output frame
 *      - QQVGA
 */
int init_internal_registers();

/**
 * @brief reset procedure, refer to daatsheet for more info
 */
int hm0360_reset();

/**
 * @brief gets the image from the camera (QQVGA)
 *
 * @param [out] buff the buffer to store the image, it needs to be of the right size
 * otherwise it's undefined behaviour
 * @note if the image appears to be cut (i.e. less columns than expected)  the MCU clock
 * is prob too slow to keep up
 */
int hm0360_get_image_164x124(uint8_t* buff);

/**
 * @brief read id of the camera via I2C, useful for testing if the comms are working
 *
 * @param [out] id the camera id
 */
void hm0360_read_id(uint16_t* id);

#endif /* __HIMAX_H */
