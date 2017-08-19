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

#ifndef MYPROJ_SSD1306_I2C_H
#define MYPROJ_SSD1306_I2C_H

#include <stdio.h>
#include <stdint.h>
#include <nrf51.h>

extern int init_oled();
extern int printAtXY(uint8_t x, uint8_t y, const char s[]);
extern int clear_screen(void);
extern int start_scroll_left(void);
extern int stop_scroll(void);
extern int set_pixel_with_scroll(uint8_t delay);

#define DISPLAY_WIDTH	128
#define DISPLAY_HEIGHT	64
#define DISPLAYSIZE		DISPLAY_WIDTH*DISPLAY_HEIGHT/8	// 8 pages/lines with 128
// 8-bit-column: 128*64 pixel
// 1024 bytes


#endif