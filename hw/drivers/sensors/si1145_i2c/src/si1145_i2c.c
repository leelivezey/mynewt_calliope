/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdbool.h>
#include <console/console.h>
#include <hal/hal_i2c.h>
#include "syscfg/syscfg.h"
#include "os/os.h"
#include "si1145_i2c/si1145_i2c.h"

int init(void);
int reset(void);

#define I2C_BUS 0

bool _debug = MYNEWT_VAL(SI1145_I2C_LOG);
uint8_t _i2c_address = 0x60;
uint8_t _i2c_channel = I2C_BUS;

struct hal_i2c_master_data i2c_data;

int i2c_write_byte(uint8_t byte, bool last_op) {
    int rc;
    uint8_t command_bytes[1];
    command_bytes[0] = byte;
    i2c_data.address = _i2c_address;
    i2c_data.buffer = command_bytes;
    i2c_data.len = 1;
    rc = hal_i2c_master_write(_i2c_channel, &i2c_data, OS_TICKS_PER_SEC, last_op);
    return rc;
}

int
write2bytes(uint8_t byte1, uint8_t byte2) {
    int rc;
    if (_debug) {
        console_printf("write %x %x to i2c \n", byte1, byte2);
    }
    uint8_t buffer[2];
    buffer[0] = byte1;
    buffer[1] = byte2;
    i2c_data.address = _i2c_address;
    i2c_data.buffer = buffer;
    i2c_data.len = 2;
    rc = hal_i2c_master_write(_i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}

int i2c_read_buffer(uint8_t *buffer, uint8_t size) {
    int rc;
    i2c_data.address = _i2c_address;
    i2c_data.buffer = buffer;
    i2c_data.len = size;
    rc = hal_i2c_master_read(_i2c_channel, &i2c_data, OS_TICKS_PER_SEC, true);
    return rc;
}

uint8_t
read_reg(uint8_t reg) {
    if (_debug) console_printf("write %x to i2c address %x \n", reg, _i2c_address);
    uint8_t buffer[1];
    int rc = i2c_write_byte(reg, true);
    if (rc != 0) {
        if (_debug) console_printf("rc = %x from i2c \n", rc);
        return -1;
    }
//  wait(0.01);
    if (_debug) console_printf("reading from i2c \n");
    if (i2c_read_buffer(buffer, 1) == 0) {
        if (_debug) console_printf("got %x from i2c \n", buffer[0]);
        return buffer[0];
    } else {
        if (_debug) console_printf("error reading from i2c \n");
        return -1;
    }
}

uint8_t
read_reg16(uint8_t reg) {
    if (_debug) console_printf("16:write %x to i2c address %x \n", reg, _i2c_address);
    uint8_t buffer[2];
    int rc = i2c_write_byte(reg, false);
    if (rc != 0) {
        if (_debug) console_printf("rc = %x from i2c \n", rc);
        return -1;
    }
//  wait(0.01);
    if (_debug) console_printf("reading two bytes from i2c \n");
    if (i2c_read_buffer(buffer, 2) == 0) {
        if (_debug) console_printf("got %x %x from i2c \n", buffer[0], buffer[1]);
        return buffer[0] | buffer[1] << 8;;
    } else {
        if (_debug) console_printf("error reading from i2c \n");
        return -1;
    }
}


int
writeParam(uint8_t p, uint8_t v) {
    write2bytes(SI1145_REG_PARAMWR, v);
    write2bytes(SI1145_REG_COMMAND, p | SI1145_PARAM_SET);
    return read_reg(SI1145_REG_PARAMRD);
}

int
readParam(uint8_t p) {
    write2bytes(SI1145_REG_COMMAND, p | SI1145_PARAM_QUERY);
    return read_reg(SI1145_REG_PARAMRD);
}

int
check_SI1145() {
    uint8_t id = read_reg(SI1145_REG_PARTID);
    if (id != 0x45) {
        return 1;
    } else {
        return 0;
    }
}

int
reset(void) {
    write2bytes(SI1145_REG_MEASRATE0, 0);
    write2bytes(SI1145_REG_MEASRATE1, 0);
    write2bytes(SI1145_REG_IRQEN, 0);
    write2bytes(SI1145_REG_IRQMODE1, 0);
    write2bytes(SI1145_REG_IRQMODE2, 0);
    write2bytes(SI1145_REG_INTCFG, 0);
    write2bytes(SI1145_REG_IRQSTAT, 0xFF);

    write2bytes(SI1145_REG_COMMAND, SI1145_RESET);
    os_time_delay(OS_TICKS_PER_SEC / 10);
    write2bytes(SI1145_REG_HWKEY, 0x17);
    os_time_delay(OS_TICKS_PER_SEC / 10);
    return 0;
}

int
init_SI1145(void) {
    int rc = 0;
#if MYNEWT_VAL(SI1145_ENABLED)
    // enable UVindex measurement coefficients!
    rc = write2bytes(SI1145_REG_UCOEFF0, 0x29);
    write2bytes(SI1145_REG_UCOEFF1, 0x89);
    write2bytes(SI1145_REG_UCOEFF2, 0x02);
    write2bytes(SI1145_REG_UCOEFF3, 0x00);

    // enable UV sensor
    writeParam(SI1145_PARAM_CHLIST, SI1145_PARAM_CHLIST_ENUV |
                                    SI1145_PARAM_CHLIST_ENALSIR | SI1145_PARAM_CHLIST_ENALSVIS |
                                    SI1145_PARAM_CHLIST_ENPS1);
    // enable interrupt on every sample
    write2bytes(SI1145_REG_INTCFG, SI1145_REG_INTCFG_INTOE);
    write2bytes(SI1145_REG_IRQEN, SI1145_REG_IRQEN_ALSEVERYSAMPLE);

/****************************** Prox Sense 1 */

    // program LED current
    write2bytes(SI1145_REG_PSLED21, 0x03); // 20mA for LED 1 only
    writeParam(SI1145_PARAM_PS1ADCMUX, SI1145_PARAM_ADCMUX_LARGEIR);
    // prox sensor #1 uses LED #1
    writeParam(SI1145_PARAM_PSLED12SEL, SI1145_PARAM_PSLED12SEL_PS1LED1);
    // fastest clocks, clock div 1
    writeParam(SI1145_PARAM_PSADCGAIN, 0);
    // take 511 clocks to measure
    writeParam(SI1145_PARAM_PSADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
    // in prox mode, high range
    writeParam(SI1145_PARAM_PSADCMISC, SI1145_PARAM_PSADCMISC_RANGE |
                                       SI1145_PARAM_PSADCMISC_PSMODE);

    writeParam(SI1145_PARAM_ALSIRADCMUX, SI1145_PARAM_ADCMUX_SMALLIR);
    // fastest clocks, clock div 1
    writeParam(SI1145_PARAM_ALSIRADCGAIN, 0);
    // take 511 clocks to measure
    writeParam(SI1145_PARAM_ALSIRADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
    // in high range mode
    writeParam(SI1145_PARAM_ALSIRADCMISC, SI1145_PARAM_ALSIRADCMISC_RANGE);

    // fastest clocks, clock div 1
    writeParam(SI1145_PARAM_ALSVISADCGAIN, 0);
    // take 511 clocks to measure
    writeParam(SI1145_PARAM_ALSVISADCOUNTER, SI1145_PARAM_ADCCOUNTER_511CLK);
    // in high range mode (not normal signal)
    writeParam(SI1145_PARAM_ALSVISADCMISC, SI1145_PARAM_ALSVISADCMISC_VISRANGE);

/************************/

    // measurement rate for auto
    write2bytes(SI1145_REG_MEASRATE0, 0xFF); // 255 * 31.25uS = 8ms

    // auto run
    write2bytes(SI1145_REG_COMMAND, SI1145_PSALS_AUTO);
#endif
    return rc;
}


// returns the UV index * 100 (divide by 100 to get the index)
uint16_t
readUV(void) {
    return read_reg16(SI1145_REG_UVINDEX0);
}

uint16_t
readVisible(void) {
    return read_reg16(SI1145_REG_ALSVISDATA0);
}

uint16_t
readIR(void) {
    return read_reg16(SI1145_REG_ALSIRDATA0);
}

