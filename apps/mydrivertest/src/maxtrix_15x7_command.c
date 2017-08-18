//
// Created by Alfred Schilken on 17.08.17.
//

#include "syscfg/syscfg.h"
#include "os/os.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <matrix_15x7/matrix_15x7.h>

#define I2C_BUS 0

static int m7x15_shell_func(int argc, char **argv);

static struct shell_cmd m15x7_cmd = {
        .sc_cmd = "m15x7",
        .sc_cmd_func = m7x15_shell_func,
};

void m7x15_command_init(void) {
    int rc;
    rc = shell_cmd_register(&m15x7_cmd);
    assert(rc == 0);
}

static int m7x15_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("m7x15 [x y i] | clr | fill | p <text>\n");
        return 1;
    }
    int rc = -1;

    if(argc == 4){
        int x;
        int y;
        int power;
        if ((sscanf( argv[1], "%d", &x) == 1) &&
                (sscanf( argv[2], "%d", &y) == 1) &&
                (sscanf( argv[3], "%d", &power) == 1) ) {
            rc = m15x7_pixel((uint8_t) x, (uint8_t) y, (uint8_t) power);
            console_printf("m15x7: set pixel %d %d to %d, rc= %d\n", x, y, power, rc);
            return 0;
        } else {
            console_printf("usage: x y power\n");
        }
    }


    if(strcmp(argv[1], "clr") == 0){
        rc = m15x7_init();
        rc = m15x7_clear();
        console_printf("m15x7 clr rc=%d\n", rc);
    }

    if (strcmp(argv[1], "fill") == 0) {
        rc = m15x7_init();
        rc = m15x7_clear();
        uint8_t level = 0;
        for (uint8_t y = 0; y < 8; y++) {
            for (uint8_t x = 0; x < 17; x++) {
                rc = m15x7_pixel((uint8_t) x, (uint8_t) y, (uint8_t) level++);
                os_time_delay(1);
            }
        }
        console_printf("m15x7 fill rc=%d\n", rc);
    }

    if(strcmp(argv[1], "p") == 0){
        if(argc > 2) {
            rc = m15x7_init();
            rc = m15x7_clear();
            rc = m15x7_print_string(argv[2]);
        }

        console_printf("m15x7 p %s rc=%d\n", argv[2], rc);
    }


    return 0;
}

