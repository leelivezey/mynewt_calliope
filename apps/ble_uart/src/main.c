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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "sysinit/sysinit.h"
#include "bsp/bsp.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "console/console.h"
#include "hal/hal_system.h"
#include "config/config.h"
#include "split/split.h"
#include <sound/sound_pwm.h>
#include <microbit_matrix/microbit_matrix.h>
#include <ws2812b/ws2812b.h>
#include <hal/hal_i2c.h>
#include "id/id.h"

#define STAMP_STR(s) STAMP_STR1(s)
#define STAMP_STR1(str) #str

#ifdef BSP_NAME
static const char *bsp_str = STAMP_STR(BSP_NAME);
static const char *app_str = STAMP_STR(APP_NAME);
#else
static const char *bsp_str = "";
static const char *app_str = "";
#endif


#define I2C_BUS 0

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "bleuart.h"

extern void sound_command_init();

static struct hal_i2c_master_data i2c_data;

static int info_ix = 0;
#define MAX_INFO_IX  2
char buf[60] = "ok\n";

/** Log data. */
struct log bleprph_log;

static int bleuart_gap_event(struct ble_gap_event *event, void *arg);

static int
send_register_and_value(uint8_t i2c_address, uint8_t reg, uint8_t value) {
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


/* ADC */
#include <adc/adc.h>
#include "adc_nrf51_driver/adc_nrf51_driver.h"

struct adc_dev *adc;
static int adc_pro_sec = 0;
static int adc_sec = 0;

static struct os_callout timer_callout;

static void adc_timer_ev_cb(struct os_event *ev) {
    assert(ev != NULL);
    adc_sample(adc);
    if(adc_pro_sec > 0) {
        os_callout_reset(&timer_callout, (adc_sec * OS_TICKS_PER_SEC) / adc_pro_sec);
    }
}

static void init_adc_timer(void) {
    os_callout_init(&timer_callout, os_eventq_dflt_get(),
                    adc_timer_ev_cb, NULL);
}

static void start_adc_timer(void) {
        os_callout_reset(&timer_callout, (adc_sec * OS_TICKS_PER_SEC) / adc_pro_sec);
}


int
adc_read_event(struct adc_dev *dev, void *arg, uint8_t etype,
               void *buffer, int buffer_len) {
    int value;
    int rc = -1;

    value = adc_nrf51_driver_read(buffer, buffer_len);
    if (value >= 0) {
        sprintf(buf, "%d\n", value);
        bleuart_uart_send_notification(buf);
//        console_printf("adc %d\n", value);
    } else {
        console_printf("Error while reading: %d\n", value);
    }
    return (rc);
}

static int
adc_init() {
    init_adc_timer();
    adc = adc_nrf51_driver_get();
    int rc = adc_event_handler_set(adc, adc_read_event, (void *) NULL);
    return rc;
}


/**
 *
  * This function will be called when the gpio_irq_handle_event is pulled
  * from the message queue.
  */
static void FUNCTION_IS_NOT_USED
ble_cmd_callback(struct os_event *ev) {
    int rc;
    int freq;
    char ch;
    console_printf(ev->ev_arg);
    int v1;
    int v2;
    int v3;
    int i2c_address;

    strcpy(buf, "OK\n");
    if (sscanf(ev->ev_arg, "s%d", &freq) == 1) {
        if (freq == 0) {
            sound_off();
        } else {
            sound_on((uint16_t) freq);
        }
    } else if (sscanf(ev->ev_arg, "c%c", &ch) == 1) {
        print_char(ch, FALSE);

    } else if (sscanf(ev->ev_arg, "b%c", &ch) == 1) {
        print_char(ch, TRUE);

    } else if (sscanf(ev->ev_arg, "l%02x%02x%02x", &v1, &v2, &v3) == 3) {
        rgb_set((uint8_t) v1, (uint8_t) v2, (uint8_t) v3);  // r g b
        rgb_send();
    } else if (sscanf(ev->ev_arg, "i%02x%02x", &i2c_address, &v2) == 2) {
//        rc = send_register_and_value((uint8_t)i2c_address, (uint8_t)v2, (uint8_t)v3);
//        sprintf(buf, "rc=%d\n", rc);
        strcpy(buf, "not yet implemented:\n");

    } else if (sscanf(ev->ev_arg, "i%02x%02x%02x", &i2c_address, &v2, &v3) == 3) {
        rc = send_register_and_value((uint8_t) i2c_address, (uint8_t) v2, (uint8_t) v3);
        sprintf(buf, "rc=%d\n", rc);

    } else if (sscanf(ev->ev_arg, "i%02x", &i2c_address) == 1) {
        rc = hal_i2c_master_probe(I2C_BUS, (uint8_t) i2c_address, OS_TICKS_PER_SEC);
        sprintf(buf, "rc=%d\n", rc);

    } else if (sscanf(ev->ev_arg, "a%d", &v1) == 1) {
        adc_pro_sec = v1;
        adc_sec = 1;
        if (v1 == 0) {
            sprintf(buf, "adc aus\n");
        } else {
            sprintf(buf, "adc %d Messungen pro sec\n", adc_pro_sec);
            start_adc_timer();
        }

    } else if (sscanf(ev->ev_arg, "A%d", &v1) == 1) {
        adc_sec = v1;
        adc_pro_sec = 1;
        if (v1 == 0) {
            sprintf(buf, "adc aus\n");
        } else {
            sprintf(buf, "adc Messungen alle %d sec\n", adc_sec);
            start_adc_timer();
        }

    } else if (strlen(ev->ev_arg) > 1 && *((char *) ev->ev_arg) == ' ') { // leerzeichen am Anfang
        print_string(ev->ev_arg + 1, TRUE);

    } else if (strlen(ev->ev_arg) <= 1) {
        switch (info_ix) {
            case 0:
                strcpy(buf, "app: ");
                strcat(buf, app_str);
                strcat(buf, "\n");
                info_ix++;
                break;
            case 1:
                strcpy(buf, "bsp:");
                strcat(buf, bsp_str);
                strcat(buf, "\n");
                info_ix++;
                break;
            case 2:
                strcpy(buf, "Kommandos:\n");
                info_ix++;
                break;
            case 3:
                strcpy(buf, "c<char> -> LED-Matrix\n");
                info_ix++;
                break;
            case 4:
                strcpy(buf, "b<char> -> blink LED-Matrix\n");
                info_ix++;
                break;
            case 5:
                strcpy(buf, "s<freq> -> sound\n");
                info_ix++;
                break;
            case 6:
                strcpy(buf, "<leerzeichen><text> -> Laufschrift\n");
                info_ix++;
                break;
            case 7:
                strcpy(buf, "i<adr> -> probe i2c adr\n");
                info_ix++;
                break;
            case 8:
                strcpy(buf, "i<adr><reg> -> read reg of i2c-adr\n");
                info_ix++;
                break;
            case 9:
                strcpy(buf, "i<adr><reg><val> -> write val into reg of i2c-adr\n");
                info_ix++;
                break;
            case 10:
                strcpy(buf, "a<n> -> adc n Messungen pro sec\n");
                info_ix++;
                break;
            case 11:
                strcpy(buf, "A<n> -> adc Messung alle n sec\n");
                info_ix++;
                break;
            case 12:
                strcpy(buf, "lrrggbb< -> Farbe der LED setzen\n");
                info_ix++;
                break;
            case 13:
                strcpy(buf, "12 Kommandos\n");
                info_ix = 0;
                break;
        }
    } else {
        strcpy(buf, "???\n");
    }
    bleuart_uart_send_notification(buf);
}

/**
 * Logs information about a connection to the console.
 */
static void
print_conn_desc(struct ble_gap_conn_desc *desc) {
    BLEPRPH_LOG(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    BLEPRPH_LOG(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    BLEPRPH_LOG(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    BLEPRPH_LOG(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    BLEPRPH_LOG(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
            "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void
bleprph_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assiging the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.uuids128 = BLE_UUID128(&gatt_svr_svc_uart_uuid.u);
    print_bytes((const uint8_t *) fields.uuids128, 16);
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        return;
    }

    memset(&fields, 0, sizeof fields);
    fields.name = (uint8_t *) ble_svc_gap_device_name();
    fields.name_len = strlen((char *) fields.name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&fields);
    if (rc != 0) {
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, bleuart_gap_event, NULL);
    if (rc != 0) {
        BLEPRPH_LOG(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleuart uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unuesd by
 *                                  bleuart.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int
bleuart_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
        BLEPRPH_LOG(INFO, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

            /* A new connection was established or a connection attempt failed. */
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                print_conn_desc(&desc);
                bleuart_set_conn_handle(event->connect.conn_handle);
            }

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                bleprph_advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            /* Connection terminated; resume advertising. */
            bleprph_advertise();
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */

            /* Delete the old bond. */
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
            assert(rc == 0);
            ble_store_util_delete_peer(&desc.peer_id_addr);

            /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
             * continue with the pairing operation.
             */
            return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    return 0;
}

static void
bleprph_on_reset(int reason) {
    BLEPRPH_LOG(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
bleprph_on_sync(void) {
    bleprph_advertise();
}

/**
 * main
 *
 * The main task for the project. This function initializes the packages,
 * then starts serving events from default event queue.
 *
 * @return int NOTE: this function should never return!
 */
int
main(void) {
    /* Initialize OS */
    sysinit();

    /* Set initial BLE device address. */
    memcpy(g_dev_addr, (uint8_t[6]) {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b}, 6);

    /* Initialize the bleprph log. */
    log_register("bleprph", &bleprph_log, &log_console_handler, NULL,
                 LOG_SYSLEVEL);

    /* Initialize the NimBLE host configuration. */
    log_register("ble_hs", &ble_hs_log, &log_console_handler, NULL,
                 LOG_SYSLEVEL);


    ble_hs_cfg.reset_cb = bleprph_on_reset;
    ble_hs_cfg.sync_cb = bleprph_on_sync;

    set_ble_cmd_cb(ble_cmd_callback);
    int rc = bleuart_gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("ble_uart");
    assert(rc == 0);

    conf_load();
    sound_command_init();
    ws2812_init();
    adc_init();
    /*
     * As the last thing, process events from default event queue.
     */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    return 0;
}
