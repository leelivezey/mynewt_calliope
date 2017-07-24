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
// Created by Alfred Schilken on 24.07.17.


#include <assert.h>
#include "sysinit/sysinit.h"
#include "os/os.h"
#include "hal/hal_gpio.h"
#include "bsp/bsp.h"
#include "sound/sound_pwm.h"


#if MYNEWT_VAL(SOUND_ENABLED)
static uint8_t sound_active = 0;
#endif

/*
static struct os_callout timer_callout;

static void timer_ev_cb(struct os_event *ev)
{
    assert(ev != NULL);
    os_callout_reset(&timer_callout, (20 * OS_TICKS_PER_SEC)/1000);
}

static void init_timer(void)
{
    os_callout_init(&timer_callout, os_eventq_dflt_get(), timer_ev_cb, NULL);
    os_callout_reset(&timer_callout, (6 * OS_TICKS_PER_SEC)/1000);
}
 */


void sound_on(uint16_t f) {
    sound_active = 1;
}
void sound_off(void){

}

void sound_silent(void){
    sound_active = 0;
}