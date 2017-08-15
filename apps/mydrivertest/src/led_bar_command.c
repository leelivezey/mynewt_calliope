//
// Created by Alfred Schilken on 25.05.17.
//

#include "syscfg/syscfg.h"
#include "os/os.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <led_bar/led_bar.h>


static int ledbar_shell_func(int argc, char **argv);

static struct shell_cmd ledbar_cmd = {
        .sc_cmd = "ledbar",
        .sc_cmd_func = ledbar_shell_func,
};

void ledbar_command_init(void) {
    int rc;
    rc = shell_cmd_register(&ledbar_cmd);
    assert(rc == 0);
}

static int ledbar_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("ledbar 0|1|2..|10\n");
        return 1;
    }
    int rc = -1;

    int value;
    char *argv1 = argv[1];
    if (argc == 2 && sscanf(argv1, "%d", &value) == 1) {
        console_printf("ledbar: value %d rc= %d\n", value, rc);
        led_bar_init();
        led_bar_set_segments(value);
        return 0;
    } else {
        console_printf("usage: ledbar <value> , value 1-10");
    }
    return 0;
}

