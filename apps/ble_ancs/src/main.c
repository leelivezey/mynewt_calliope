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
#include <host/ble_uuid.h>
#include "sysinit/sysinit.h"
#include "bsp/bsp.h"
#include "os/os.h"
#include "bsp/bsp.h"
#include "hal/hal_gpio.h"
#include "console/console.h"
#include "hal/hal_system.h"
#include "config/config.h"

/* BLE */
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

/* Application-specified header. */
#include "bleprph.h"

#include "peer.h"


static const ble_uuid128_t ancs_base_uuid128 =
        BLE_UUID128_INIT(0xd0, 0x00, 0x2d, 0x12, 0x1e, 0x4b, 0x0f, 0xa4,
                         0x99, 0x4e, 0xce, 0xb5, 0x31, 0xf4, 0x05, 0x79);

static const ble_uuid128_t notification_source_uuid =
        BLE_UUID128_INIT(0xbd, 0x1d, 0xa2, 0x99, 0xe6, 0x25, 0x58, 0x8c,
                         0xd9, 0x42, 0x01, 0x63, 0x0d, 0x12, 0xbf, 0x9f);

static const ble_uuid128_t control_point_uuid =
        BLE_UUID128_INIT(0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98,
                         0xa8, 0x49, 0xe1, 0x45, 0xf3, 0xd8, 0xd1, 0x69);

static const ble_uuid128_t data_source_uuid128 =
        BLE_UUID128_INIT(0xfb, 0x7b, 0x7c, 0xce, 0x6a, 0xb3, 0x44, 0xbe,
                         0xb5, 0x4b, 0xd6, 0x24, 0xe9, 0xc6, 0xea, 0x22);

//const UUID ancs_uuid("7905F431B5CE4E99A40F4B1E122D00D0");
//const UUID notification_source_uuid("9F BF 12 0D 63 01 42 D9 8C 58 25 E6 99 A2 1D BD");
//const UUID       control_point_uuid("69 D1 D8 F3 45 E1 49 A8 98 21 9B BD FD AA D9 D9");
//const UUID         data_source_uuid("22 EA C6 E9 24 D6 4B B5 BE 44 B3 6A CE 7C 7B FB");

/** Log data. */
struct log bleprph_log;

static int bleprph_gap_event(struct ble_gap_event *event, void *arg);

/**
 * Logs information about a connection to the console.
 */
static void
bleprph_print_conn_desc(struct ble_gap_conn_desc *desc) {
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

    fields.uuids128 = BLE_UUID128(&ancs_base_uuid128.u);
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
                           &adv_params, bleprph_gap_event, NULL);
    if (rc != 0) {
        BLEPRPH_LOG(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

#if 0
static void start_discovery() {
/**
 * Performs the Limited or General Discovery Procedures.
 *
 * @param own_addr_type         The type of address the stack should use for
 *                                  itself when sending scan requests.  Valid
 *                                  values are:
 *                                      o BLE_ADDR_TYPE_PUBLIC
 *                                      o BLE_ADDR_TYPE_RANDOM
 *                                      o BLE_ADDR_TYPE_RPA_PUB_DEFAULT
 *                                      o BLE_ADDR_TYPE_RPA_RND_DEFAULT
 *                                  This parameter is ignored unless active
 *                                  scanning is being used.
 * @param duration_ms           The duration of the discovery procedure.
 *                                  On expiration, the procedure ends and a
 *                                  BLE_GAP_EVENT_DISC_COMPLETE event is
 *                                  reported.  Units are milliseconds.  Specify
 *                                  BLE_HS_FOREVER for no expiration.
 * @param disc_params           Additional arguments specifying the particulars
 *                                  of the discovery procedure.
 * @param cb                    The callback to associate with this discovery
 *                                  procedure.  Advertising reports and
 *                                  discovery termination events are reported
 *                                  through this callback.
 * @param cb_arg                The optional argument to pass to the callback
 *                                  function.
 *
 * @return                      0 on success; nonzero on failure.
 */
    struct ble_gap_disc_params disc_params = {0};
    disc_params.filter_duplicates = 1;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 20000, &disc_params, bleprph_gap_event, NULL);
    BLEPRPH_LOG(INFO, "ble_gap_disc rc: %d\n", rc);
}
#endif

/**
 * Called when service discovery of the specified peer has completed.
 */
static void
blecent_on_disc_complete(const struct peer *peer, int status, void *arg)
{

    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        BLEPRPH_LOG(ERROR, "Error: Service discovery failed; status=%d "
                "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    BLEPRPH_LOG(ERROR, "Service discovery complete; status=%d "
            "conn_handle=%d\n", status, peer->conn_handle);

    /* Now perform three concurrent GATT procedures against the peer: read,
     * write, and subscribe to notifications.
     */
//    blecent_read_write_subscribe(peer);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unuesd by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int
bleprph_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;
    char s[BLE_HS_ADV_MAX_SZ];

    BLEPRPH_LOG(INFO, "bleprph_gap_event %d\n", event->type);

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed. */
            BLEPRPH_LOG(INFO, "connection %s; status=%d ",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                bleprph_print_conn_desc(&desc);
            }
            BLEPRPH_LOG(INFO, "\n");

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                bleprph_advertise();
            } else {
                /* Remember peer. */
                rc = peer_add(event->connect.conn_handle);
                if (rc != 0) {
                    BLEPRPH_LOG(ERROR, "Failed to add peer; rc=%d\n", rc);
                    return 0;
                }

                /* Perform service discovery. */
                rc = peer_disc_all(event->connect.conn_handle,
                                   blecent_on_disc_complete, NULL);
                if (rc != 0) {
                    BLEPRPH_LOG(ERROR, "Failed to discover services; rc=%d\n", rc);
                    return 0;
                }
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            BLEPRPH_LOG(INFO, "disconnect; reason=%d ", event->disconnect.reason);
            bleprph_print_conn_desc(&event->disconnect.conn);
            BLEPRPH_LOG(INFO, "\n");

            /* Connection terminated; resume advertising. */
            bleprph_advertise();
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            /* The central has updated the connection parameters. */
            BLEPRPH_LOG(INFO, "connection updated; status=%d ",
                        event->conn_update.status);
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
            BLEPRPH_LOG(INFO, "\n");
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            /* Encryption has been enabled or disabled for this connection. */
            BLEPRPH_LOG(INFO, "encryption change event; status=%d ",
                        event->enc_change.status);
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
            BLEPRPH_LOG(INFO, "\n");
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            BLEPRPH_LOG(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                        event->subscribe.conn_handle,
                        event->subscribe.attr_handle,
                        event->subscribe.reason,
                        event->subscribe.prev_notify,
                        event->subscribe.cur_notify,
                        event->subscribe.prev_indicate,
                        event->subscribe.cur_indicate);
            return 0;

        case BLE_GAP_EVENT_MTU:
            BLEPRPH_LOG(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.channel_id,
                        event->mtu.value);
            return 01;

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

        case BLE_GAP_EVENT_DISC:

            rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                         event->disc.length_data);
            BLEPRPH_LOG(INFO, "BLE_GAP_EVENT_DISC rc=%d, flags=0x%02x\n", rc, fields.flags);
            if (rc != 0) {
                return 0;
            }

            if (fields.svc_data_uuid128 != NULL) {
                BLEPRPH_LOG(INFO, "    svc_data_uuid128=");
                print_bytes(fields.svc_data_uuid128, fields.svc_data_uuid128_len);
                BLEPRPH_LOG(DEBUG, "\n");
            }
            if (fields.name != NULL) {
                assert(fields.name_len < sizeof s - 1);
                memcpy(s, fields.name, fields.name_len);
                s[fields.name_len] = '\0';
                BLEPRPH_LOG(INFO, "    name(%scomplete)=%s\n",
                            fields.name_is_complete ? "" : "in", s);
            }
            if (fields.uuids128 != NULL) {
                BLEPRPH_LOG(INFO, "    uuids128(%scomplete)=",
                            fields.uuids128_is_complete ? "" : "in");
                for (int i = 0; i < fields.num_uuids128; i++) {
                    print_uuid(&fields.uuids128[i].u);
                    BLEPRPH_LOG(INFO, " ");
                }
                BLEPRPH_LOG(DEBUG, "\n");
            }

    }

    return 0;
}

static void
bleprph_on_reset(int reason) {
    BLEPRPH_LOG(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
bleprph_on_sync(void) {
    /* Begin advertising. */
//    start_discovery();
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
    int rc;

    /* Set initial BLE device address. */
    memcpy(g_dev_addr, (uint8_t[6]) {0x0b, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c}, 6);

    /* Initialize OS */
    sysinit();

    /* Initialize the bleprph log. */
    log_register("bleprph", &bleprph_log, &log_console_handler, NULL,
                 LOG_SYSLEVEL);

    /* Initialize the NimBLE host configuration. */
    log_register("ble_hs", &ble_hs_log, &log_console_handler, NULL,
                 LOG_SYSLEVEL);
    ble_hs_cfg.reset_cb = bleprph_on_reset;
    ble_hs_cfg.sync_cb = bleprph_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Initialize data structures to track connected peers. */
    rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 5, 3, 1);
    assert(rc == 0);

    rc = gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("nimble-ancs");
    assert(rc == 0);

    conf_load();

    /*
     * As the last thing, process events from default event queue.
     */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    return 0;
}
