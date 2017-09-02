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

#ifndef _BLEUART_H_
#define _BLEUART_H_

#include "log/log.h"
#include "nimble/ble.h"
#include "host/ble_uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct log bleprph_log;

/* bleprph uses the first "peruser" log module. */
#define BLEPRPH_LOG_MODULE  (LOG_MODULE_PERUSER + 0)

/* Convenience macro for logging to the bleprph module. */
#define BLEPRPH_LOG(lvl, ...) \
    LOG_WARN(&bleprph_log, BLEPRPH_LOG_MODULE, __VA_ARGS__)

void set_ble_cmd_cb(os_event_fn *ev_cb);

int bleuart_svc_register(void);
int bleuart_gatt_svr_init(void);
void bleuart_set_conn_handle(uint16_t conn_handle);

extern const ble_uuid128_t gatt_svr_svc_uart_uuid;

/** Misc. */
void print_bytes(const uint8_t *bytes, int len);
void print_addr(const void *addr);

#ifdef __cplusplus
}
#endif

#endif /* _BLEUART_H */
