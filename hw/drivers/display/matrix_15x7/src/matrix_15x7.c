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
#include <matrix_15x7/matrix_15x7.h>
#include <matrix_15x7/font_5x7.h>

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

int m15x7_init(){
    int rc;
    i2c_data.address = i2c_address;
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x00);
    // delay(10);
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_SHUTDOWN, 0x01);
    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);

    rc = send_function_with_param(ISSI_BANK_FUNCTIONREG, ISSI_REG_PICTUREFRAME, _page);
    for (uint8_t page=0; page<8; page++) {
        for (uint8_t i=0; i<=0x11; i++)
            send_function_with_param(page, i, 0xff);     // each 8 LEDs on
    }
    return rc;
}

int m15x7_clear(void) {
    int rc = -1;
    for(uint8_t x = 0; x < 16; x++ ) {
        for (uint8_t y = 0; y < 9; y++) {
            rc = m15x7_pixel((uint8_t) x, (uint8_t) y, (uint8_t) 0);
        }
    }
    return rc;
}

/*
 * x/y in second parameter of function send_function_with_param()
 *
 * 7/0 7/1 ... 7/7  8/7 8/6 ... 8/1
 * 6/0 6/1     6/7  9/7 9/6 ... 9/1
 * ...              ...
 * ...              ...
 * ...              ...
 * 1/0 1/1 ... 1/7  14/7 ....  14/1
 */

int
m15x7_pixel(uint8_t x, uint8_t y, uint8_t power) {
    int rc = 0;
    if ((x < 0) || (x > 14) || (y < 0) || (y > 6)) {
        return -1;
    }
    if (x > 7) {
        x=15-x;
        y += 8;
    } else {
        y = 7-y;
    }
    send_function_with_param(_page, 0x24+(y + x*16), power);
    return rc;
}

int m15x7_print_char(char ch, uint8_t ix) {
    uint8_t xpos = ix * 5;
    const uint8_t *p_pixeldata;
    if(ch == '\0') {
        return -1;
    }
    if(ch >= ' ' && ch <= 0x7f) {
        p_pixeldata = &font5x7[(ch - ' ')*5]; // skip first 32 chars, one char is 5 bytes
    } else {
        p_pixeldata = &font5x7[sizeof(font5x7) - 5]; // use last char as fallback
    }
    for(uint8_t x = 0; x < 5; x++ ) {
        for (uint8_t y = 0; y < 7; y++) {
            if(p_pixeldata[x] & (1 << y)){
                m15x7_pixel(xpos + x, y, 40);
            }
        }
    }
    return 0;
}


int m15x7_print_string(char* text) {
    m15x7_clear();
    for(int ix = 0; ix < 3; ix++){
        m15x7_print_char(text[ix], ix);
    }
    return 0;
}




