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
//

#include "os/os_eventq.h"
#include <stdint.h>

#ifndef MYPROJ_SOUND_PWM_H
#define MYPROJ_SOUND_PWM_H

//default values
#define CALLIOPE_SM_DEFAULT_DUTY_M                      50
#define CALLIOPE_SM_DEFAULT_DUTY_S                      100
#define CALLIOPE_SM_DEFAULT_FREQUENCY_S                 4000
#define CALLIOPE_SM_DEFAULT_SILENT_MODE                 1

//constants
#define CALLIOPE_SM_PRESCALER_M                         2
#define CALLIOPE_SM_PRESCALER_S                         0
#define CALLIOPE_SM_PRESCALER_S_LF                      4           //prescaler for creating low frequencies
#define CALLIOPE_SM_PERIOD_M                            100
#define CALLIOPE_MIN_FREQUENCY_HZ_S_NP                  245         //min possible frequency due to 16bit timer resolution (without prescaler)
#define CALLIOPE_MIN_FREQUENCY_HZ_S                     20          //min human audible frequency
#define CALLIOPE_MAX_FREQUENCY_HZ_S                     20000       //max human audible frequency
#define CALLIOPE_BOARD_FREQUENCY                        (16000000)

extern void sound_on(uint16_t f);
extern void sound_off(void);
extern void sound_silent(bool on_off);

#endif //MYPROJ_SOUND_PWM_H
