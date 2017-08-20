//
// Created by Alfred Schilken on 20.08.17.
//

#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include "os/os.h"
#include "bsp/bsp.h"
#include <hal/hal_i2c.h>

static int i2c_scan_cmd(int argc, char **argv);

static int i2c_get_cmd(int argc, char **argv);

static int i2c_set_cmd(int argc, char **argv);

#define I2C_MODULE "i2c"
#define I2C_BUS 0

/*
 * Initialize param and command help information if SHELL_CMD_HELP
 * is enabled.
 */

#if MYNEWT_VAL(SHELL_CMD_HELP)
static const struct shell_param i2c_get_params[] = {
        {"", "register"},
        {NULL, NULL}
};

static const struct shell_cmd_help i2c_get_help = {
        .summary = "get register to value using i2c",
        .usage = NULL,
        .params = i2c_get_params,
};

static const struct shell_param i2c_set_params[] = {
        {"", "register"},
        {"", "value"},
        {NULL, NULL}
};

static const struct shell_cmd_help i2c_set_help = {
        .summary = "set register to value using i2c",
        .usage = NULL,
        .params = i2c_set_params,
};

static const struct shell_cmd_help i2c_scan_help = {
        .summary = "scan i2c bus",
        .usage = NULL,
        .params = NULL,
};
#endif

/*
 * Initialize an array of shell_cmd structures for the commands
 * in the i2c module.
 */
static const struct shell_cmd i2c_module_commands[] = {
        {
                .sc_cmd = "set",
                .sc_cmd_func = i2c_set_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
                .help = &i2c_set_help,
#endif
        },
        {
                .sc_cmd = "get",
                .sc_cmd_func = i2c_get_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
                .help = &i2c_get_help,
#endif
        },
        {
                .sc_cmd = "scan",
                .sc_cmd_func = i2c_scan_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
                .help = &i2c_scan_help,
#endif
        },
        {NULL, NULL, NULL},
};


void
i2c_commands_init(void) {
    int rc = shell_register(I2C_MODULE, i2c_module_commands);
    assert(rc == 0);
}


static int
i2c_scan_cmd(int argc, char **argv) {
    int address;
    int rc;
    for (address = 0; address < 0x80; address++) {
        rc = hal_i2c_master_probe(I2C_BUS, (uint8_t) address, OS_TICKS_PER_SEC / 10);
        if (rc == 0) {
            console_printf("i2c: scan %02x rc= %d\n", address, rc);
        }
    }
    return 0;
}


static int
i2c_set_cmd(int argc, char **argv) {
    int i2c_reg = -1;
    int i2c_val = -1;
    if (argc != 3) {
        console_printf("usage: i2c s <reg> <val>, i.e. s 40 ff\n");
        return 1;
    }
    if (sscanf(argv[1], "%x", &i2c_reg) != 1) {
        return -1;
    }
    if (sscanf(argv[2], "%x", &i2c_val) != 1) {
            return 1;
    }
    console_printf("i2c: s %02x %02x\n", i2c_reg, i2c_val);
    return 0;
}

static int
i2c_get_cmd(int argc, char **argv) {
    int i2c_reg = -1;
    int i2c_count = -1;
    if (argc != 3) {
        console_printf("usage: i2c g <reg> <count>, i.e. g 40 1\n");
        return 1;
    }
    if (sscanf(argv[1], "%x", &i2c_reg) != 1) {
        return -1;
    }
    if (sscanf(argv[2], "%x", &i2c_count) != 1) {
        return 1;
    }
    console_printf("i2c: g %02x %02x\n", i2c_reg, i2c_count);
    return 0;
}
