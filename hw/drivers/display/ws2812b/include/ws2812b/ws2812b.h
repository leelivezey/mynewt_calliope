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
// Created by Alfred Schilken on 18.07.17.
//

#ifndef MYPROJ_WS2812B_H
#define MYPROJ_WS2812B_H

#include <stdint.h>

extern void ws2812_init();
extern void rgb_set(uint8_t r, uint8_t g, uint8_t b);
extern void rgb_send();

// map separate vars g, r and b to an array with 3 bytes
typedef union {
    struct {
        uint8_t g, r, b;
    }asVars;
    uint8_t asArray[3];
} grb_color_t;

//These defines are timed specific to a series of if statements and will need to be changed
//to compensate for different writing algorithms than the one in neopixel.c
#define WS2812B_SEND_ONE	NRF_GPIO->OUTSET = (1UL << WS2812B_LED_PIN); \
			__ASM ( \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
				); \
			NRF_GPIO->OUTCLR = (1UL << WS2812B_LED_PIN); \


#define WS2812B_SEND_ZERO 	NRF_GPIO->OUTSET = (1UL << WS2812B_LED_PIN); \
			__ASM (  \
					" NOP\n\t"  \
				);  \
			NRF_GPIO->OUTCLR = (1UL << WS2812B_LED_PIN); \
			__ASM ( \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
				);

#define WS2812B_SEND_ZERO_NO_IRQ   						 sr = __get_PRIMASK(); \
        							  				    __disable_irq();      \
			NRF_GPIO->OUTSET = (1UL << WS2812B_LED_PIN); \
			__ASM (  \
					" NOP\n\t"  \
				);  \
			NRF_GPIO->OUTCLR = (1UL << WS2812B_LED_PIN); \
															if (!sr) { __enable_irq(); } \
			__ASM ( \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
					" NOP\n\t" \
				);


#endif //MYPROJ_WS2812B_H
