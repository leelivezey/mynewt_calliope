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

#include "syscfg/syscfg.h"
#include "hal/hal_gpio.h"
#include "led_bar/led_bar.h"
#include "nrf51.h"

#define LOW         0
#define HIGH        1

#define ON          0x00FF
#define OFF         0x0000
#define CMDMODE     0x0000   //Work on 8-bit mode

#define CLOCK_PIN   (MYNEWT_VAL(LED_BAR_CLOCK_PIN))
#define DATA_PIN    (MYNEWT_VAL(LED_BAR_DATA_PIN))

void
led_bar_init() {
    hal_gpio_init_out(CLOCK_PIN, 0);
    hal_gpio_init_out(DATA_PIN, 0);
    led_bar_set_segments(0); // all OFF
}

static void
nrf51_delay_us(uint32_t number_of_us)
{
    register uint32_t delay __ASM ("r0") = number_of_us;
    __ASM volatile (
    ".syntax unified\n"
            "1:\n"
            " SUBS %0, %0, #1\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " NOP\n"
            " BNE 1b\n"
            ".syntax divided\n"
    : "+r" (delay));
}

static void
latchData()
{
    hal_gpio_write(DATA_PIN, LOW);
    nrf51_delay_us(250);

    for(int i=0; i<4; i++)
    {
        hal_gpio_write(DATA_PIN, HIGH);
        hal_gpio_write(DATA_PIN, LOW);
        nrf51_delay_us(1);
    }
}

static void
send16bitData(uint16_t data)
{
    for(int i=0; i<16; i++)
    {
        int state = (data & 0x8000) ? HIGH : LOW;
        hal_gpio_write(DATA_PIN, state);
        hal_gpio_toggle(CLOCK_PIN);
        data <<= 1;
    }
}

// set led single bit, red to green, one bit for each led
// such as, index_bits = 0x05, then led 1 and led 3 will be on and the others will be off
// 0x0   = 0b000000000000000 = all leds off
// 0x05  = 0b000000000000101 = leds 1 and 3 on, the others off
// 0x155 = 0b000000101010101 = leds 1,3,5,7,9 on, 2,4,6,8,10 off
// 0x3ff = 0b000001111111111 = all leds on
void
led_bar_set_segments(uint16_t index_bits)
{
    send16bitData(CMDMODE);

    for (int i=0;i<12;i++)
    {
        uint16_t data_bits = (index_bits & 0x0001) ? ON : OFF;
        send16bitData(data_bits);
        index_bits = index_bits>>1;
    }
    latchData();
}









