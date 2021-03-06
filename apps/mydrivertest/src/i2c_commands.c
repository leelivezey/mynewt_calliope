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
static int i2c_toggle_cmd(int argc, char **argv);

#define I2C_MODULE "i2c"
#define I2C_BUS 0
static struct hal_i2c_master_data i2c_data;

static uint8_t last_i2c_address;
static uint8_t last_i2c_reg = 0xff;

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
static const struct shell_cmd_help i2c_toggle_help = {
        .summary = "toggle last adr/reg 10000 times",
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
        {
            .sc_cmd = "toggle",
            .sc_cmd_func = i2c_toggle_cmd,
#if MYNEWT_VAL(SHELL_CMD_HELP)
            .help = &i2c_toggle_help,
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
send_register_and_value(uint8_t i2c_address, uint8_t reg, uint8_t value){
    int rc;
    uint8_t command_bytes[2];
    command_bytes[0] = reg;
    command_bytes[1] = value;
    i2c_data.address = i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
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
    int i2c_addr = -1;
    int i2c_reg = -1;
    int i2c_val = -1;
    int i2c_val2 = -1;
    int rc = -1;
    if (argc < 4) {
        console_printf("usage: i2c set <addr> <reg> <val> [<val2>], i.e. s 48 40 ff\n");
        return 1;
    }
    if (sscanf(argv[1], "%x", &i2c_addr) != 1) {
        return -1;
    }
    if (sscanf(argv[2], "%x", &i2c_reg) != 1) {
        return -1;
    }
    if (sscanf(argv[3], "%x", &i2c_val) != 1) {
            return 1;
    }

    if(argc == 5){
        if (sscanf(argv[4], "%x", &i2c_val2) != 1) {
            return 1;
        }
        uint8_t command_bytes[3];
        command_bytes[0] = i2c_reg;
        command_bytes[1] = i2c_val;
        command_bytes[2] = i2c_val2;
        i2c_data.address = i2c_addr;
        i2c_data.buffer = command_bytes;
        i2c_data.len = 3;
        rc = hal_i2c_master_write(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
    } else {
        last_i2c_address = i2c_addr;
        last_i2c_reg = i2c_reg;
        rc = send_register_and_value((uint8_t)i2c_addr, (uint8_t)i2c_reg, (uint8_t)i2c_val);
    }
    if (rc == 0) {
        console_printf("i2c: addr: %02x -> set reg %02x to value %02x\n", i2c_addr, i2c_reg, i2c_val);
    } else {
        console_printf("i2c: rc=%02x\n", rc);
    }
    return 0;
}

static int
i2c_get_cmd(int argc, char **argv) {
    int i2c_addr = -1;
    int i2c_reg = -1;
    int i2c_reg2 = -1;
    int i2c_count = -1;
    int rc = -1;
    if (argc < 4) {
        console_printf("usage: i2c get <addr> <reg> [<reg2>] <count>, i.e. g 48 40 1\n");
        return 1;
    }
    if (sscanf(argv[1], "%x", &i2c_addr) != 1) {
        return -1;
    }
    if (sscanf(argv[2], "%x", &i2c_reg) != 1) {
        return -1;
    }

    uint8_t command_bytes[9];
    i2c_data.address = i2c_addr;
    i2c_data.buffer = command_bytes;

    if(argc == 5) {
        if (sscanf(argv[3], "%x", &i2c_reg2) != 1) {
            return -2;
        }
        if (sscanf(argv[4], "%x", &i2c_count) != 1) {
            return -2;
        }
        command_bytes[0] = i2c_reg;
        command_bytes[1] = i2c_reg2;
        i2c_data.len = 2;
        rc = hal_i2c_master_write(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
        if(rc != 0){
            console_printf("i2c: write rc=%02x\n", rc);
            return -4;
        }
    } else if(argc == 4){
        if (sscanf(argv[3], "%x", &i2c_count) != 1) {
            return -2;
        }
        command_bytes[0] = i2c_reg;
        i2c_data.len = 1;
        rc = hal_i2c_master_write(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
        if(rc != 0){
            console_printf("i2c: write rc=%02x\n", rc);
            return -4;
        }
    }
    if(i2c_count > 8){
        console_printf("usage: i2c get:  0 < count <= 8\n");
        return -3;
    }
    memset(command_bytes, 0x55, sizeof(command_bytes));
    i2c_data.len = i2c_count;
    rc = hal_i2c_master_read(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
    if (rc == 0) {
        console_printf("i2c: get from reg %02x %02x bytes\n", i2c_reg, i2c_count);
        for(int ix = 0; ix < i2c_count; ix++){
            console_printf("%02x ", command_bytes[ix]);
        }
        console_printf("\n");
    } else {
        console_printf("i2c: read rc=%02x\n", rc);
        return -5;
    }

    return 0;
}

#define LOOP_COUNT 10000

static int
i2c_toggle_cmd(int argc, char **argv) {
    int rc = -1;
    uint8_t command_bytes[3];
    command_bytes[0] = last_i2c_reg;
    command_bytes[1] = 1;
    i2c_data.address = last_i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 2;
    uint32_t start_cputime = os_cputime_get32();
    for(int nn = 0; nn < LOOP_COUNT; nn++){
        rc = hal_i2c_master_write(I2C_BUS, &i2c_data, OS_TICKS_PER_SEC, true);
        command_bytes[1] ^= 0x01;
    }
    uint32_t end_cputime = os_cputime_get32();
    console_printf("%d times: start: %d, end: %d, delta: %d\n", LOOP_COUNT, (int)start_cputime, (int)end_cputime, (int)(end_cputime-start_cputime));
    return rc;
}
