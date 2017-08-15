//
// Created by Alfred Schilken on 28.05.17.
//
#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <hal/hal_i2c.h>
#include <ssd1306_i2c/ssd1306_i2c.h>

static int oled_shell_func(int argc, char **argv);

static struct shell_cmd oled_cmd = {
        .sc_cmd = "oled",
        .sc_cmd_func = oled_shell_func,
};

void oled_command_init(void) {
    int rc;
    rc = shell_cmd_register(&oled_cmd);
    assert(rc == 0);
}

static int oled_shell_func(int argc, char **argv) {
    int rc;
    if (argc < 2) {
        console_printf("oled clr|p <string>\n");
        return 1;
    }

    if(strcmp(argv[1], "init") == 0) {
        rc = init_oled();
        console_printf("oled: init rc= %d\n", rc);
        return 0;
    }

    if(strcmp(argv[1], "p") == 0){
        char* argv2 = argv[2];
        if (argc > 2) {
            rc = printAtXY(3, 0, argv2);
            return 0;
        } else {
            console_printf("usage: oled p <text>");
        }
    }

    if(strcmp(argv[1], "clr") == 0){
        rc = clear_screen();
        console_printf("oled: clear screen rc= %d\n", rc);
        return 0;
    }

    /*
    if(strcmp(argv[1], "inv") == 0){
        rc = send_command_byte(0xA7);
        console_printf("oled: invert screen rc= %d\n", rc);
        return 0;
    }
     */
    return 0;
}


