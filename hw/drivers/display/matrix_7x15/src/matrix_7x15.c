/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
//
// Created by Alfred Schilken on 15.08.17.
//

#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include "os/os.h"
#include <hal/hal_i2c.h>
#include "ssd1306_i2c/font_8x16.h"
#include <ssd1306_i2c/ssd1306_i2c.h>

#define ISSI_ADDR_DEFAULT 0x74


#define ISSI_REG_CONFIG  0x00
#define ISSI_REG_CONFIG_PICTUREMODE 0x00
#define ISSI_REG_CONFIG_AUTOPLAYMODE 0x08
#define ISSI_REG_CONFIG_AUDIOPLAYMODE 0x18


#define ISSI_CONF_PICTUREMODE 0x00
#define ISSI_CONF_AUTOFRAMEMODE 0x04
#define ISSI_CONF_AUDIOMODE 0x08

#define ISSI_REG_PICTUREFRAME  0x01

#define ISSI_REG_SHUTDOWN 0x0A
#define ISSI_REG_AUDIOSYNC 0x06

#define ISSI_COMMANDREGISTER 0xFD
#define ISSI_BANK_FUNCTIONREG 0x0B    // helpfully called 'page nine'


#define I2C_BUS 0

static uint8_t i2c_channel = I2C_BUS;
static uint8_t i2c_address = 0x74;

uint8_t _page = 0;

struct hal_i2c_master_data i2c_data;

extern const uint8_t font8x16[];

/*
static int
send_data_bytes(uint8_t bytes[], uint8_t size){
    int rc;
    uint8_t command_bytes[9];
    command_bytes[0] = 0x40;
    memcpy(&command_bytes[1], bytes, size);
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = size+1;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}
*/

static int
send_function_with_param(uint8_t page, uint8_t function_code, uint8_t param){
    int rc;
    uint8_t command_bytes[2];
    command_bytes[0] = ISSI_COMMANDREGISTER;
    command_bytes[1] = page;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);

    command_bytes[0] = function_code;
    command_bytes[1] = param;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);

    return rc;
}

int m7x15_init(){
    int rc;
    i2c_data.address = i2c_address;
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x00);
    // delay(10);
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x01);
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);

    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_PICTUREFRAME, _page);
    return rc;
}

int m7x15_clear(void) {
    int rc = -1;
    return rc;
}

int
m7x15_pixel(uint8_t x, uint8_t y, uint8_t power) {
    int rc = 0;
    send_function_with_param(_page, 0x24+(x + y*16), power);
    return rc;
}





