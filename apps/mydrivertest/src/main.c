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
#include <si1145_i2c/si1145_i2c.h>
#include <ssd1306_i2c/ssd1306_i2c.h>
#include <buttons/button_polling.h>

#if MYNEWT_VAL(MATRIX_15X7_ENABLED)
#include "matrix_15x7/matrix_15x7.h"
#endif

#ifdef ARCH_sim
#include "mcu/mcu_sim.h"
#endif

static volatile int g_task1_loops;

int g_led_pin;

extern void gpio_commands_init(void);
extern void i2c_command_init(void);
extern void oled_command_init(void);
extern void uv_command_init(void);
extern void matrix_command_init(void);
extern void rgb_command_init(void);
extern void adc_commands_init(void);
extern void sound_command_init(void);
extern void ledbar_command_init(void);
extern void m7x15_command_init(void);

static struct os_callout blinky_callout;

static char ch = ' ';

static bool oled_initilized = false;


#if MYNEWT_VAL(SI1145_ENABLED)
static char print_buffer[20];

static void
readAndPrintUV() {
    uint16_t uv = readUV();
    sprintf(print_buffer, "UV-Index: %d.%02d", uv/100, uv%100);
    printAtXY(1, 1, print_buffer);
}
#endif

static void initOled() {
    int rc = -1;
    if (!oled_initilized) {
        rc = init_oled();
        if (rc == 0) {
            oled_initilized = true;
            rc = clear_screen();
        }
    }
}

/**
  * This function will be called when the gpio_irq_handle_event is pulled
  * from the message queue.
  */
static void FUNCTION_IS_NOT_USED
button_callback(struct os_event *ev)
{
    if((int)ev->ev_arg == BUTTON_A_PIN) {
        ch++;
        print_char(ch, FALSE);
    } else {
#if MYNEWT_VAL(SI1145_ENABLED)
        readAndPrintUV();
#endif
        ch--;
        print_char(ch, TRUE);
    }

#if MYNEWT_VAL(MATRIX_15X7_ENABLED)
    char buf[4];
    sprintf(buf, "%c%c%c", (char)ch-1, (char)ch, (char)ch+1);
    m15x7_print_string(buf);
#endif

}

// Event callback function for timer events. It toggles the led pin.

static void
timer_ev_cb(struct os_event *ev) {
    assert(ev != NULL);
    ++g_task1_loops;
    hal_gpio_toggle(g_led_pin);
    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
}

static void init_timer(void) {
// Initialize the callout for a timer event.
    os_callout_init(&blinky_callout, os_eventq_dflt_get(), timer_ev_cb, NULL);
    os_callout_reset(&blinky_callout, OS_TICKS_PER_SEC);
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
    init_timer();
    gpio_commands_init();
    i2c_command_init();
    oled_command_init();
    uv_command_init();
    matrix_command_init();
    rgb_command_init();
    adc_commands_init();
    sound_command_init();
    ledbar_command_init();
    m7x15_command_init();
//    shell_register_default_module("");
    initOled();
    printAtXY(1, 1, "UV+OLED v0.7");
    printAtXY(1, 4, "Button B fuer  neue Messung");

#if MYNEWT_VAL(BUTTON_LOG)
    log_register("button", &_log, &log_console_handler, NULL, LOG_SYSLEVEL);
#endif
    microbit_set_button_cb(button_callback);
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    assert(0);
    return rc;
}

