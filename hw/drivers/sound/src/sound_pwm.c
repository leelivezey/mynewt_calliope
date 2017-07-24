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
#include "nrf51_bitfields.h"

#if MYNEWT_VAL(SOUND_ENABLED)
static uint8_t sound_active = 0;
static bool silent_mode = false;
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


__STATIC_INLINE void nrf_gpiote_task_enable(uint32_t idx)
{
    uint32_t final_config = NRF_GPIOTE->CONFIG[idx] | GPIOTE_CONFIG_MODE_Task;
    /* Workaround for the OUTINIT PAN. When nrf_gpiote_task_config() is called a glitch happens
    on the GPIO if the GPIO in question is already assigned to GPIOTE and the pin is in the
    correct state in GPIOTE but not in the OUT register. */
    /* Configure channel to Pin31, not connected to the pin, and configure as a tasks that will set it to proper level */
    NRF_GPIOTE->CONFIG[idx] = final_config | ((31 << GPIOTE_CONFIG_PSEL_Pos) & GPIOTE_CONFIG_PSEL_Msk);
    __NOP();
    __NOP();
    __NOP();
    NRF_GPIOTE->CONFIG[idx] = final_config;
}

__STATIC_INLINE void nrf_gpiote_task_disable(uint32_t idx)
{
    NRF_GPIOTE->CONFIG[idx] &= ~GPIOTE_CONFIG_MODE_Task;
}

/**
 * @enum nrf_gpiote_polarity_t
 * @brief Polarity for the GPIOTE channel.
 */
typedef enum
{
    NRF_GPIOTE_POLARITY_LOTOHI = GPIOTE_CONFIG_POLARITY_LoToHi,       ///<  Low to high.
    NRF_GPIOTE_POLARITY_HITOLO = GPIOTE_CONFIG_POLARITY_HiToLo,       ///<  High to low.
    NRF_GPIOTE_POLARITY_TOGGLE = GPIOTE_CONFIG_POLARITY_Toggle        ///<  Toggle.
} nrf_gpiote_polarity_t;

/**
* @enum nrf_gpiote_outinit_t
* @brief Initial output value for the GPIOTE channel.
*/
typedef enum
{
    NRF_GPIOTE_INITIAL_VALUE_LOW  = GPIOTE_CONFIG_OUTINIT_Low,       ///<  Low to high.
    NRF_GPIOTE_INITIAL_VALUE_HIGH = GPIOTE_CONFIG_OUTINIT_High       ///<  High to low.
} nrf_gpiote_outinit_t;

__STATIC_INLINE void nrf_gpiote_task_configure(uint32_t idx, uint32_t pin,
                                               nrf_gpiote_polarity_t polarity,
                                               nrf_gpiote_outinit_t  init_val)
{
    NRF_GPIOTE->CONFIG[idx] &= ~(GPIOTE_CONFIG_PSEL_Msk |
                                 GPIOTE_CONFIG_POLARITY_Msk |
                                 GPIOTE_CONFIG_OUTINIT_Msk);

    NRF_GPIOTE->CONFIG[idx] |= ((pin << GPIOTE_CONFIG_PSEL_Pos) & GPIOTE_CONFIG_PSEL_Msk) |
                               ((polarity << GPIOTE_CONFIG_POLARITY_Pos) & GPIOTE_CONFIG_POLARITY_Msk) |
                               ((init_val << GPIOTE_CONFIG_OUTINIT_Pos) & GPIOTE_CONFIG_OUTINIT_Msk);
}

void sound_on(uint16_t f) {
    if((f > CALLIOPE_MAX_FREQUENCY_HZ_S) || (f < CALLIOPE_MIN_FREQUENCY_HZ_S)) return;
    sound_active = 1;
//stop & clear timer
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //set prescaler for sound use
    if(f < CALLIOPE_MIN_FREQUENCY_HZ_S_NP) {
        NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_S_LF;
    } else {
        NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_S;
    }
    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set pins to default values
    NRF_GPIO->OUTCLR = (1UL << CALLIOPE_PIN_MOTOR_IN1);
    NRF_GPIO->OUTSET = (1UL << CALLIOPE_PIN_MOTOR_IN2);
//    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
//    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);

    //max 50% duty per pwm just like in dual motor use
    uint8_t duty = (uint8_t)(CALLIOPE_SM_DEFAULT_DUTY_S/2);

    //calculate period corresponding to the desired frequency and the currently used prescaler
    uint16_t period;
    if(f < CALLIOPE_MIN_FREQUENCY_HZ_S_NP) {
        period = (uint16_t)(((uint32_t)CALLIOPE_BOARD_FREQUENCY) / ((uint32_t)f << CALLIOPE_SM_PRESCALER_S_LF));
    } else {
        period = (uint16_t)(((uint32_t)CALLIOPE_BOARD_FREQUENCY) / (uint32_t)f);
    }

    //set compare register 2 and 3 according to the gives frequency (this sets the PWM period)
    NRF_TIMER2->CC[2] = period-1;
    NRF_TIMER2->CC[3] = period;

    //set duty cycle
    NRF_TIMER2->CC[0] = period - (uint16_t)((uint32_t)(period * duty) / 100);
    NRF_TIMER2->CC[1] = (uint16_t)((uint32_t)((period * duty) / 100)) - 1;

    //enable task & restart PWM depending on silent mode setting
    nrf_gpiote_task_enable(0);
    if(!silent_mode) nrf_gpiote_task_enable(1);
    NRF_TIMER2->TASKS_START = 1;

    //activate controller
    NRF_GPIO->OUTSET = (1UL << CALLIOPE_PIN_MOTOR_SLEEP);
//    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);

}
void sound_off(void){
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    NRF_GPIO->OUTCLR = (1UL << CALLIOPE_PIN_MOTOR_IN1);
    NRF_GPIO->OUTCLR = (1UL << CALLIOPE_PIN_MOTOR_IN2);

    NRF_GPIO->OUTCLR = (1UL << CALLIOPE_PIN_MOTOR_SLEEP);
}

void sound_silent(bool on_off){
    sound_active = 0;
    silent_mode = on_off;
}

void PWM_init() {
    hal_gpio_init_out(CALLIOPE_PIN_MOTOR_IN1, 0);
    hal_gpio_init_out(CALLIOPE_PIN_MOTOR_IN2, 0);
    hal_gpio_init_out(CALLIOPE_PIN_MOTOR_SLEEP, 0);

    //create tasks to perform on timer compare match
    NRF_GPIOTE->POWER = 1;

    nrf_gpiote_task_configure(0, CALLIOPE_PIN_MOTOR_IN1, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_task_enable(0);
    //task 1
    nrf_gpiote_task_configure(1, CALLIOPE_PIN_MOTOR_IN2, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_enable(1);
    //Three NOPs are required to make sure configuration is written before setting tasks or getting events
    __NOP();
    __NOP();
    __NOP();

//connect the tasks to the corresponding compare match events, toggle twice per period (PWM)
    //connect task 0
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
    //connect task 1
    NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[1];
    NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

    //connect task 0
    NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[2];
    NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
    //connect task 1
    NRF_PPI->CH[3].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[3];
    NRF_PPI->CH[3].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

    NRF_PPI->CHENSET = 15; // bits 0 - 3 for channels 0 - 3

    //init TIMER2 for PWM use
    NRF_TIMER2->POWER = 1;
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_M;
    NRF_TIMER2->TASKS_CLEAR = 1;

//initialize compare registers
    //set compare registers 0 and 1 (duty cycle for PWM on pins CALLIOPE_PIN_MOTOR_IN1 and CALLIOPE_PIN_MOTOR_IN2)
    NRF_TIMER2->CC[0] = CALLIOPE_SM_PERIOD_M - CALLIOPE_SM_DEFAULT_DUTY_M;
    NRF_TIMER2->CC[1] = CALLIOPE_SM_DEFAULT_DUTY_M-1;
    //set compare register 2 and 3 (period for PWM on pins CALLIOPE_PIN_MOTOR_IN1 and CALLIOPE_PIN_MOTOR_IN2)
    NRF_TIMER2->CC[2] = CALLIOPE_SM_PERIOD_M-1;
    NRF_TIMER2->CC[3] = CALLIOPE_SM_PERIOD_M;
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
}

