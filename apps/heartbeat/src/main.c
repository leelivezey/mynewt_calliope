/**
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

#include <assert.h>
#include <string.h>
#include "syscfg/syscfg.h"
#include "sysinit/sysinit.h"
#include <shell/shell.h>
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include <microbit_matrix/microbit_matrix.h>
#include <buttons/button_polling.h>
#include <console/console.h>

#ifdef ARCH_sim
#include "mcu/mcu_sim.h"
#endif

static volatile int g_task1_loops;

int g_led_pin;

static struct os_callout blinky_callout;

static char ch = ' ';
static bool running = false;
static int new_irq_millisec = 3;
static int last_irq_millisec;

#define SAMPLES_DELAY    3
#define GROVE_A1_P1      26

/**
  * This function will be called when the gpio_irq_handle_event is pulled
  * from the message queue.
  */
static void FUNCTION_IS_NOT_USED
button_callback(struct os_event *ev)
{
    if((int)ev->ev_arg == BUTTON_A_PIN) {
        running = !running;
        ch = (running) ? '>' : '_';
        print_char(ch, running);
    } else {
        print_char(ch, TRUE);
    }
}

// Event callback function for timer events. It toggles the led pin.

static char buffer[2];

static void
timer_ev_cb(struct os_event *ev) {
    assert(ev != NULL);
    ++g_task1_loops;
    int32_t ticks = 1;
    if (running) {
        ticks = SAMPLES_DELAY;
//        buffer[0] = hal_gpio_read(GROVE_A1_P1) ? '1' : '0';
//        console_write(buffer, sizeof(buffer));
        if(new_irq_millisec != last_irq_millisec) {
            buffer[0] = '1';
 //           console_printf("%d\n", new_irq_millisec);
            last_irq_millisec = new_irq_millisec;
        } else {
            buffer[0] = '0';
        }
        console_write(buffer, sizeof(buffer));
    } else {
        hal_gpio_toggle(g_led_pin);
    }
    os_callout_reset(&blinky_callout, ticks);
}

static void init_timer(void) {
// Initialize the callout for a timer event.
    os_callout_init(&blinky_callout, os_eventq_dflt_get(), timer_ev_cb, NULL);
    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
}


void interruptHandler(void *unused) {
    new_irq_millisec = os_get_uptime_usec() / 1000;
}


/**
 * main
 *
 * The main task for the project. This function initializes packages,
 * and then blinks the BSP LED in a loop.
 *
 * @return int NOTE: this function should never return!
 */
int
main(int argc, char **argv)
{
    int rc;
#ifdef ARCH_sim
    mcu_sim_parse_args(argc, argv);
#endif
    sysinit();
    g_led_pin = LED_COL2;
    hal_gpio_init_out(LED_ROW1, 1);
    hal_gpio_init_out(g_led_pin, 0);
    hal_gpio_init_in(GROVE_A1_P1, HAL_GPIO_PULL_DOWN);

    rc = hal_gpio_irq_init(GROVE_A1_P1, interruptHandler, NULL,
                           HAL_GPIO_TRIG_RISING, HAL_GPIO_PULL_DOWN);
    hal_gpio_irq_enable(GROVE_A1_P1);
    assert(rc == 0);
    buffer[1] = '\n';
    init_timer();

    microbit_set_button_cb(button_callback);
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    assert(0);
    return rc;
}

