//
// Created by Alfred Schilken on 25.05.17.
//

#include "syscfg/syscfg.h"
#include "os/os.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <hal/hal_i2c.h>

#define I2C_BUS 0

static int i2c_shell_func(int argc, char **argv);

static struct shell_cmd i2c_cmd = {
        .sc_cmd = "i2c",
        .sc_cmd_func = i2c_shell_func,
};

void i2c_command_init(void) {
    int rc;
    rc = shell_cmd_register(&i2c_cmd);
    assert(rc == 0);
}

static int i2c_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("i2c probe|scan\n");
        return 1;
    }
    int rc;

    if(strcmp(argv[1], "probe") == 0){
        int address;
        char* argv2 = argv[2];
        if (argc > 2 && sscanf( argv2, "%02x", &address) == 1) {
            rc = hal_i2c_master_probe(I2C_BUS, (uint8_t)address, OS_TICKS_PER_SEC);
            console_printf("i2c: probe %x rc= %d\n", address, rc);
            return 0;
        } else {
            console_printf("usage: i2c probe <address>");
        }
    }

    if(strcmp(argv[1], "scan") == 0){
        int address;
        for (address = 0; address < 0x80; address++) {
            rc = hal_i2c_master_probe(I2C_BUS, (uint8_t)address, OS_TICKS_PER_SEC/10);
            if ( rc == 0) {
                console_printf("i2c: probe %02x rc= %d\n", address, rc);
            }
        }
    }

    return 0;
}

