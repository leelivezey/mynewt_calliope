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

#include "microbit_matrix/microbit_matrix.h"
#include "microbit_matrix/font_5x5.h"

#include <assert.h>
#include <stdio.h>
#include "os/os.h"

static const char *bit_rep[16] = {
        [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
        [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
        [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
        [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

static const const uint8_t digit_pattern[10]= {
        0b00000,
        0b00001,
        0b00011,
        0b00111,
        0b01111,
        0b11111,
        0b11110,
        0b11100,
        0b11000,
        0b10000
};

#define FMT8BITS(byte) bit_rep[(byte) >> 4], bit_rep[(byte) & 0x0F]

// erste Stelle: row
// zweite Stelle: column
static uint8_t xy2rowcol[] = { 0x00, 0x13, 0x01, 0x14, 0x02,
                               0x23, 0x24, 0x25, 0x26, 0x27,
                               0x11, 0x08, 0x12, 0x28, 0x10,
                               0x07, 0x06, 0x05, 0x04, 0x03,
                               0x22, 0x16, 0x20, 0x15, 0x21 };

static uint16_t visible_row[5];
static uint32_t gpioBitfieldMask = ROW_MASK | COL_MASK;
static uint32_t gpioBitfieldValue;
static uint8_t current_row = 2;
static uint16_t ticks = 0;
static uint16_t shift_counter = 0;

static char* _start_ptr;
static char* _current_ptr;
static boolean _loop = FALSE;
static boolean _scroll = FALSE;
static boolean _blink = FALSE;
static boolean _refreshing = false;
static boolean _blink_phase_on = false;
boolean microbit_matrix_log = false;

static __IO uint32_t *reg_cnf;
static __IO uint32_t *reg_out;
static __I  uint32_t *reg_in;

static struct os_callout refresh_callout;

static void led_matrix_refresh();
static void
timer_ev_cb(struct os_event *ev) {
    assert(ev != NULL);
    led_matrix_refresh();
    if(_refreshing) {
        os_callout_reset(&refresh_callout, OS_TICKS_PER_SEC / 150);
    }
}

static void init_timer(void) {
    if(!_refreshing) {
        os_callout_init(&refresh_callout, os_eventq_dflt_get(), timer_ev_cb, NULL);
        os_callout_reset(&refresh_callout, OS_TICKS_PER_SEC);
        _refreshing = true;
    }
}

static void config_gpio_output(uint32_t bitmask){
uint8_t i;
reg_out = &NRF_GPIO->OUT;
reg_in  = &NRF_GPIO->IN;
reg_cnf = NRF_GPIO->PIN_CNF;
for (i = 0; i<31; i++) {
if (bitmask & (1 << i)) {
reg_cnf[i] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
| (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
}
}
}

static void copy_bits_to_visible_rows(char ch) {
    uint8_t y;
    uint16_t start_index_in_pixel_array = ((uint8_t) ch - 0x020) * BYTES_PER_CHAR;
    const uint8_t *pixels = &font_as_bytes[start_index_in_pixel_array];

    for (y = 0; y < BYTES_PER_CHAR; y++) {
        visible_row[y] &= ~0x1f;            // clear lower 5 bits
        visible_row[y] |= (pixels[y] & 0x1f);        // paste pixels of char
    }
}

static void init_timer_and_gpio(){
    config_gpio_output(ROW_MASK | COL_MASK);
    init_timer();
}

static void shift_left(){
    uint8_t y;
    for ( y = 0; y < 5; y++ ) {
        visible_row[y] <<= 1;
    }
    if(++shift_counter % 6 == 0){
        if(*_current_ptr) {
            copy_bits_to_visible_rows(*_current_ptr++);
        } else {
            if(_loop){
                _current_ptr = _start_ptr;
            } else {
                _refreshing = FALSE;
            }
        }
    }
}

/*
static  void  dump_visible_rows(){
    uint8_t y;
    for ( y = 0; y < 5; y++ ) {
        printf("%s%s%s%s\n", FMT8BITS(visible_row[y] >> 8), FMT8BITS(visible_row[y] & 0xff) );
    }
}
*/


static void write_gpio_register(){
    if(microbit_matrix_log) {
        printf("%d: %s%s %s%s %s%s %s%s\n", current_row,
               FMT8BITS(gpioBitfieldValue >> 24),
               FMT8BITS(gpioBitfieldValue >> 16),
               FMT8BITS(gpioBitfieldValue >> 8),
               FMT8BITS(gpioBitfieldValue & 0xff)
        );
    }
    *reg_out = (*reg_out & ~gpioBitfieldMask) | (gpioBitfieldValue & gpioBitfieldMask);
}

void display_off(){
    gpioBitfieldValue = 1 << (current_row + GPIO_OF_ROW_0) | COL_MASK;
    write_gpio_register();
}

static void led_matrix_refresh() {
    uint8_t col, row, rowcol, y, x;
    current_row++;
    ticks++;
    if (current_row > 2) current_row = 0;
    gpioBitfieldValue = 1 << (current_row + GPIO_OF_ROW_0) | COL_MASK;

    if(_blink && ticks % 100 == 0) {
        _blink_phase_on = !_blink_phase_on;
    }
    if(!_blink_phase_on) {
        display_off();
        return;
    }
    for (y = 0; y < 5; y++) {
        for (x = 0; x < 5; x++) {
            if (visible_row[y] & (0x200 >> x)) { // bit links oben im zweiten 5x5-block von links
                rowcol = xy2rowcol[y * 5 + x];
                row = rowcol >> 4;     // 0 bis 2
                if (row == current_row) {
                    col = rowcol & 0x0f;    // 0 bis 8
                    gpioBitfieldValue ^= (1 << (col + GPIO_OF_COL_0));
                }
            }
        }
    }
    write_gpio_register();
    if(_scroll && (ticks % 28 == 0)){
        shift_left();
    }
}

void clearPixels() {
    for (uint8_t y = 0; y < BYTES_PER_CHAR; y++) {
        visible_row[y] &= ~0b1111100000;            // clear lower 5 bits
    }
}

void showIntAs5Digits(uint16_t value){
    clearPixels();
    for (int ix = 0; ix < 5; ix++) {
        uint8_t columnPixels = digit_pattern[value % 10];
        value = value / 10;
        for(uint8_t y = 0; y < 5; y++){
            if (columnPixels & (0x10 >> y)) {
                visible_row[y] |= (0x20 << ix);
            }
        }
    }
    _blink_phase_on = true;
    _scroll = FALSE;
    _blink = false;
    init_timer_and_gpio();
}

int set_pixel_at_xy(uint8_t x, uint8_t y){
    visible_row[y] |= (0x200 >> x);
    return 0;
}

int print_char(char ch, boolean blink) {
    copy_bits_to_visible_rows(ch);
    _blink = blink;
    _blink_phase_on = true;
    _scroll = FALSE;
    uint8_t y;
    for ( y = 0; y < 5; y++ ) {
        visible_row[y] <<= 5;
    }
    init_timer_and_gpio();
    return 0;
}

int print_string(char* text, boolean loop){
    _start_ptr = text;
    _current_ptr = text;
    _loop = loop;
    _scroll = TRUE;
    _blink_phase_on = true;
    _blink = false;
    init_timer_and_gpio();
    return 0;
}

bool isScrolling() {
    return *_current_ptr && !_loop;
}