//
// Created by Alfred Schilken on 25.05.17.
//

#include "syscfg/syscfg.h"
#include "os/os.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <matrix_7x15/matrix_7x15.h>

#define I2C_BUS 0

static int m7x15_shell_func(int argc, char **argv);

static struct shell_cmd m7x15_cmd = {
        .sc_cmd = "m7x15",
        .sc_cmd_func = m7x15_shell_func,
};

void m7x15_command_init(void) {
    int rc;
    rc = shell_cmd_register(&m7x15_cmd);
    assert(rc == 0);
}

static int m7x15_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("m7x15 x y i\n");
        return 1;
    }
    int rc = -1;

    if(strcmp(argv[1], "init") == 0){
        rc = m7x15_init();
        rc = m7x15_clear();
        rc = m7x15_pixel((uint8_t)2, (uint8_t)5, (uint8_t)20);
        console_printf("m7x15 init rc=%d\n", rc);
    }

    return 0;
}

