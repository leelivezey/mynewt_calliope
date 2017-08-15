//
// Created by Alfred Schilken on 25.05.17.
//

#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"

static int gpio_scan_cmd(int argc, char **argv);
static int gpio_write_cmd(int argc, char **argv);

#define GPIO_MODULE "gpio"

static int8_t pins_to_scan[] = {
    0,        // Stern
    1,        // Stern
    2,        // Stern
    22,       // Stern
    26, 27,   // Grove B
    17,       // Button A
    16,       // Button B
    3         // Microphon
//    19, 20, // Grove A
//    18,     // RGB LED
};

static uint32_t previous_value = 0;

static int gpio_loop = 0;

// /* gpio Task settings */
#define GPIO_TASK_PRIO           100
#define GPIO_STACK_SIZE          (OS_STACK_ALIGN(128))
struct os_task gpio_task;
bssnz_t os_stack_t gpio_stack[GPIO_STACK_SIZE];

static char buf[32];

static void
init_all_pins() {
    for(int ix = 0; ix < sizeof(pins_to_scan); ix++){
        hal_gpio_init_in(pins_to_scan[ix], HAL_GPIO_PULL_NONE);
    }
}

static void
read_and_compare_all_pins() {
    for(int ix = 0; ix < sizeof(pins_to_scan); ix++){
        int pin = pins_to_scan[ix];
        uint32_t  mask = 0x01 << pin;
        if(hal_gpio_read(pin)) {
            if((previous_value & mask) == 0) {
                sprintf(buf, "pin %d now 1\n", pin);
                console_printf(buf);
            }
            previous_value |= mask;
        } else {
            if((previous_value & mask) != 0) {
                sprintf(buf, "pin %d now 0\n", pin);
                console_printf(buf);
            }
            previous_value &= ~mask;
        }
    }
}

static void
gpio_task_handler(void *unused)
{
//    int rc;
    while (1) {
        if(gpio_loop){
//            int tick = (int)os_time_get();
//            sprintf(buf, "%d ", tick);
//            console_printf(buf);
            read_and_compare_all_pins();
        }
        os_time_delay(OS_TICKS_PER_SEC / 10);
    }
}

/*
 * Initialize param and command help information if SHELL_CMD_HELP
 * is enabled.
 */

#if MYNEWT_VAL(SHELL_CMD_HELP)
static const struct shell_param gpio_write_params[] = {
    {"", "pin"},
    {"", "value"},
    {NULL, NULL}
};

static const struct shell_cmd_help gpio_write_help = {
    .summary = "write to gpio",
    .usage = NULL,
    .params = gpio_write_params,
};


static const struct shell_cmd_help gpio_scan_help = {
    .summary = "scan calliope gpios",
    .usage = NULL,
    .params = NULL,
};

#endif

/*
 * Initialize an array of shell_cmd structures for the commands
 * in the gpio module.
 */
static const struct shell_cmd gpio_module_commands[] = {
    {
        .sc_cmd = "w",
        .sc_cmd_func = gpio_write_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
        .help = &gpio_write_help,
#endif
    },
    {
        .sc_cmd = "scan",
        .sc_cmd_func = gpio_scan_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
        .help = &gpio_scan_help,
#endif
    },
    { NULL, NULL, NULL },
};


void
gpio_commands_init(void)
{
    int rc = shell_register(GPIO_MODULE, gpio_module_commands);
    assert(rc == 0);
    os_task_init(&gpio_task, "gpio", gpio_task_handler,
                 NULL, GPIO_TASK_PRIO, OS_WAIT_FOREVER,
                 gpio_stack, GPIO_STACK_SIZE);

}

#ifdef BBC_MICROBIT
int8_t gpio_mapping[21] = {
        3,
        2,
        1,
        4,
        5,
        17,
        12,
        11,
        18,
        10,
        6,
        26,
        20,
        23,
        22,
        21,
        16,
        -1,
        -1,
        0,
        30
};

int getPinFromGpioPin(int8_t gpioPin){
    int8_t pin = -1;
    int8_t numPins = sizeof(gpio_mapping);
    for(pin = 0; pin < numPins; pin++){
        if(gpio_mapping[pin] == gpioPin){
            break;
        }
    }
    return pin < numPins ? pin : -1;
}
#endif


static int
gpio_scan_cmd(int argc, char **argv) {
    int onOff = 0;
    if (argc != 2) {
        console_printf("usage: gpio scan 0|1\n");
        return 1;
    }
    if (sscanf( argv[1], "%d", &onOff) != 1) {
        if(onOff != 0 && onOff != 1) {
            console_printf("0 für OFF, 1 für ON\n");
            return 1;
        }
    }
    gpio_loop = onOff;
    init_all_pins();
    console_printf("gpio: scan %s\n", onOff == 1 ? "ON" : "OFF");
    console_printf("%d anzahl getesteter Pins\n", sizeof(pins_to_scan));
    return 0;
}

static int
gpio_write_cmd(int argc, char **argv) {
    int onOff;
    int gpio_pin=-1;
    char* pin;
    if (argc != 3) {
        console_printf("usage: gpio w <pin> <onOff>, <pin>: p0..p30, onOff: 0 oder 1\n");
        return 1;
    }
    pin = argv[1];
    if (*pin != 'P' && *pin != 'p') {
        console_printf("<pin>: p0..p30\n");
        return 1;
    }
#ifdef BBC_MICROBIT
    if (sscanf(pin, "P%d", &pin_nr) == 1) {
        if(pin_nr < 0 || pin_nr > 20) {
            console_printf("Board-PIN: 0 bis 20\n");
            return 1;
        }
        gpio_pin = gpio_mapping[pin_nr];
    }
#endif
    if (sscanf( pin, "p%d", &gpio_pin) == 1) {
        if(gpio_pin < 0 || gpio_pin > 30) {
            console_printf("CPU-pin: 0 bis 30\n");
            return 1;
        }
#ifdef BBC_MICROBIT
        int pin_nr = getPinFromGpioPin((int8_t)gpio_pin);
#endif
    }
    if (sscanf( argv[2], "%d", &onOff) != 1) {
        if(onOff != 0 && onOff != 1) {
            console_printf("0 für LOW(0V), 1 für HIGH (3V)\n");
            return 1;
        }
    }
#ifdef BBC_MICROBIT
    console_printf("gpio: P%d(p%d) %s\n", pin_nr, gpio_pin, onOff == 0 ? "LOW" : "HIGH");
#else
    console_printf("gpio: p%d %s\n", gpio_pin, onOff == 0 ? "LOW" : "HIGH");
#endif
//    hal_gpio_init_out(gpio_pin, onOff);
    return 0;
}
