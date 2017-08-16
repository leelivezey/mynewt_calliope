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
    if (argc < 3) {
        console_printf("ledbar [b <bitmask 0-3ff>] | [v <volume 0-100> \n");
        return 1;
    }
    int value;
    char *argv1 = argv[1];
    if(*argv1 == 'b') {
        if (argc == 3 && sscanf(argv[2], "%03x", &value) == 1) {
            led_bar_init();
            led_bar_set_segments(value);
            console_printf("ledbar: b %03x\n", value);
            return 0;
        } else {
            console_printf("usage: ledbar b <bitmask> , <bitmask 0-3ff>\n");
        }
    }
    if(*argv1 == 'v') {
        if (argc == 3 && sscanf(argv[2], "%d", &value) == 1) {
            led_bar_init();
            led_bar_set_level(value);
            console_printf("ledbar: v %d\n", value);
            return 0;
        } else {
            console_printf("usage: ledbar <value> , value 0-100\n");
        }
    }
    return 0;
}

