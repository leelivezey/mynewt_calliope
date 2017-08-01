//
// Created by Alfred Schilken on 29.05.17.
//
#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include "syscfg/syscfg.h"
#include <assert.h>
#include <stdio.h>
#include <si1145_i2c/si1145_i2c.h>

static int uv_shell_func(int argc, char **argv);

static struct shell_cmd uv_cmd = {
        .sc_cmd = "uv",
        .sc_cmd_func = uv_shell_func,
};

void uv_command_init(void) {
    int rc;
    rc = shell_cmd_register(&uv_cmd);
    assert(rc == 0);
}

static int uv_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("uv check|r\n");
        return 1;
    }
    int rc;

    if(strcmp(argv[1], "check") == 0){
#if MYNEWT_VAL(SI1145_ENABLED)
        rc = check_SI1145();
#else
        rc = -1;
        console_printf("uv: SI1145_ENABLED not set, rc= %d\n", rc);
#endif
        console_printf("uv: check UV-sensor SI1145 on i2c rc= %d\n", rc);
        return 0;
    }

    if(strcmp(argv[1], "r") == 0){
        uint16_t uv = readUV();
        uint16_t visible = readVisible();
        uint16_t ir = readIR();
        console_printf("uv: UV index = %d.%02d\n", uv/100, uv%100);
        console_printf("uv: Visible value= %d \n", visible);
        console_printf("uv: IR value= %d\n", ir);
        return 0;
    }

    return 0;
}



