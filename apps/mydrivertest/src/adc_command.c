//
// Created by Alfred Schilken on 04.08.17.
//

#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include "bsp/bsp.h"
#include "os/os.h"
#include <assert.h>
#include <stdio.h>
#include <microbit_matrix/microbit_matrix.h>

static int adc_loop_cmd(int argc, char **argv);
static int adc_poll_cmd(int argc, char **argv);

#define ADC_MODULE "adc"

/* ADC */
#include <adc/adc.h>
#include "adc_nrf51_driver/adc_nrf51_driver.h"

// /* ADC Task settings */
#define ADC_TASK_PRIO           5
#define ADC_STACK_SIZE          (OS_STACK_ALIGN(32))
struct os_task adc_task;
bssnz_t os_stack_t adc_stack[ADC_STACK_SIZE];

static int adc_loop = 0;
static int previous_value = -1;

int
adc_handle_event(struct adc_dev *dev, void *arg, uint8_t etype,
                                  void *buffer, int buffer_len) {
    int value;
    int rc = -1;

    value = adc_nrf51_driver_read(buffer, buffer_len);
    if (value >= 0) {
        showIntAs5Digits(value);
        if (!isScrolling()) {
            if(adc_loop && (previous_value != value)){
                console_printf("%4d ", value);
            }
        }
    } else {
        console_printf("Error while reading: %d\n", value);
    }
    previous_value = value;
    return (rc);
}

static void
adc_task_handler(void *unused)
{
    struct adc_dev *adc;
    int rc;
    /* ADC init */
    adc = adc_nrf51_driver_get();
    rc = adc_event_handler_set(adc, adc_handle_event, (void *) NULL);
    assert(rc == 0);

    while (1) {
        adc_sample(adc);
        /* Wait 1/2 second */
        os_time_delay(OS_TICKS_PER_SEC / 2);
    }
}


/*
 * Initialize param and command help information if SHELL_CMD_HELP
 * is enabled.
 */

#if MYNEWT_VAL(SHELL_CMD_HELP)
static const struct shell_param adc_loop_params[] = {
    {"", "on/off 0|1"},
    {NULL, NULL}
};

static const struct shell_cmd_help adc_loop_help = {
    .summary = "adc loop",
    .usage = NULL,
    .params = adc_loop_params,
};


static const struct shell_cmd_help adc_poll_help = {
    .summary = "adc poll",
    .usage = NULL,
    .params = NULL,
};

#endif

/*
 * Initialize an array of shell_cmd structures for the commands
 * in the adc module.
 */
static const struct shell_cmd adc_module_commands[] = {
    {
        .sc_cmd = "loop",
        .sc_cmd_func = adc_loop_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
        .help = &adc_loop_help,
#endif
    },
    {
        .sc_cmd = "poll",
        .sc_cmd_func = adc_poll_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
        .help = &adc_poll_help,
#endif
    },
    { NULL, NULL, NULL },
};


void
adc_commands_init(void)
{
    int rc = shell_register(ADC_MODULE, adc_module_commands);
    assert(rc == 0);
    os_task_init(&adc_task, "adc", adc_task_handler,
                 NULL, ADC_TASK_PRIO, OS_WAIT_FOREVER,
                 adc_stack, ADC_STACK_SIZE);
}


static int
adc_poll_cmd(int argc, char **argv) {
    console_printf("%d \n", argc);
    return 0;
}

static int
adc_loop_cmd(int argc, char **argv) {
    int onOff = 0;
    if (argc != 2) {
        console_printf("usage: adc loop 0|1\n");
        return 1;
    }
    if (sscanf( argv[1], "%d", &onOff) != 1) {
        if(onOff != 0 && onOff != 1) {
            console_printf("0 für OFF, 1 für ON\n");
            return 1;
        }
    }
    adc_loop = onOff;
    console_printf("adc: loop %s\n", onOff == 1 ? "ON" : "OFF");
    return 0;
}
