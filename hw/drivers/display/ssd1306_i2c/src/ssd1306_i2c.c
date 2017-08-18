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
// Created by Alfred Schilken on 28.05.17.
//

#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include "os/os.h"
#include <hal/hal_i2c.h>
#include "ssd1306_i2c/font_8x16.h"
#include <ssd1306_i2c/ssd1306_i2c.h>

#define I2C_BUS 0

#define SSD1306_NORMALDISPLAY           0xA6
#define SSD1306_INVERTDISPLAY           0xA7

#define SSD1306_ACTIVATE_SCROLL         0x2F
#define SSD1306_DEACTIVATE_SCROLL       0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL  0x27

static uint8_t i2c_channel = I2C_BUS;
static uint8_t i2c_address = 0x3c;

struct hal_i2c_master_data i2c_data;

extern const uint8_t font8x16[];

static const uint8_t ssd1306_init_sequence []  = {	// Initialization Sequence
        0xAE,			// Display OFF (sleep mode)
        0x20, 0x00,		// Set Memory Addressing Mode
        // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
        // 10=Page Addressing Mode (RESET); 11=Invalid
        0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
        0xC8,			// Set COM Output Scan Direction
        0x00,			// ---set low column address
        0x10,			// ---set high column address
        0x40,			// --set start line address
        0x81, 0x3F,		// Set contrast control register
        0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
        0xA6,			// Set display mode. A6=Normal; A7=Inverse
        0xA8, 0x3F,		// Set multiplex ratio(1 to 64)
        0xA4,			// Output RAM to Display
        // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
        0xD3, 0x00,		// Set display offset. 00 = no offset
        0xD5,			// --set display clock divide ratio/oscillator frequency
        0xF0,			// --set divide ratio
        0xD9, 0x22,		// Set pre-charge period
        0xDA, 0x12,		// Set com pins hardware configuration
        0xDB,			// --set vcomh
        0x20,			// 0x20,0.77xVcc
        0x8D, 0x14,		// Set DC-DC enable
        0xAF			// Display ON in normal mode
};

static int
setPositionXY(uint8_t x, uint8_t y) {
    int rc;
    uint8_t command_bytes[5];
    command_bytes[0] = 0x00;
    command_bytes[1] = 0xb0 + y;
    command_bytes[2] = 0x21;
    command_bytes[3] = x;
    command_bytes[4] = 0x7f;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 5;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}

/*
static int
send_data_byte(uint8_t byte){
    int rc;
    uint8_t command_bytes[2];
    command_bytes[0] = 0x40;
    command_bytes[1] = byte;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}
*/

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

int clear_screen(void) {
    int rc;
    uint16_t i;
    rc = setPositionXY(0, 0);
    uint8_t buffer[8] = { 0,0,0,0,0,0,0,0};
    for (i = 0; i < DISPLAYSIZE; i++) {
        send_data_bytes(buffer, 8);
    }
    setPositionXY(0, 0);
    return rc;
}

static int
send_command_byte(uint8_t byte){
    int rc;
    uint8_t command_bytes[2];
    command_bytes[0] = 0x00;
    command_bytes[1] = byte;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}

int init_oled(){
    int rc;
    i2c_data.address = i2c_address;
    i2c_data.buffer = (uint8_t*)&ssd1306_init_sequence;
    i2c_data.len = sizeof(ssd1306_init_sequence);
    rc = hal_i2c_master_write(i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}

int
printAtXY(uint8_t x, uint8_t y, const char s[]) {
    uint8_t ch, j = 0;
    int rc = 0;

    while (s[j] != '\0') {
        ch = s[j] - 32;
        if (x > 120) {
            x = 0;
            y += 2;
        }
        rc = setPositionXY(x, y);

        send_data_bytes((uint8_t*)&font8x16[ch * 16], 8);
        setPositionXY(x, y + 1);
        send_data_bytes((uint8_t*)&font8x16[ch * 16 + 8], 8);
        x += 8;
        j++;
    }
    return rc;
}

int
start_scroll_left(){
    send_command_byte(SSD1306_LEFT_HORIZONTAL_SCROLL);
    send_command_byte(0X00);
    send_command_byte(0);
    send_command_byte(0X00);
    send_command_byte(15);
    send_command_byte(0X00);
    send_command_byte(0XFF);
    send_command_byte(SSD1306_ACTIVATE_SCROLL);
    return 0;
}

int
stop_scroll() {
    send_command_byte(SSD1306_DEACTIVATE_SCROLL);
    return 0;
}




