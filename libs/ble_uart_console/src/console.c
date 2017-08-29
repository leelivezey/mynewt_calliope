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
#include <inttypes.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "syscfg/syscfg.h"
#include "os/os.h"
#include "sysinit/sysinit.h"
#include "console/console.h"

#define MAX_LINES_QUEUED 4
#define MAX_LINE_LENGTH 60

//MYNEWT_VAL(BLEUART_MAX_LINELEN)

static struct os_eventq avail_queue;
static struct os_eventq lines_queue;
static char* buf[MAX_LINES_QUEUED];
static struct os_event line_available_ev[MAX_LINES_QUEUED];

/* Indicates whether the previous line of output was completed. */
//
//int console_is_midline;

static console_rx_cb console_compat_rx_cb; /* callback that input is ready */

char tttbuf[80] = "alles ok";

int
console_printf(const char *fmt, ...)
{
    return 0;
}

void console_write(const char *str, int cnt){
    static struct os_event *ev;
    static char *buf;

    ev = os_eventq_get_no_wait(&avail_queue);
    if (!ev) {
        return;
    }
    buf = ev->ev_arg;
    if( cnt > MAX_LINE_LENGTH-1) {
        cnt = MAX_LINE_LENGTH-1;
    }
    strncpy(buf, str, cnt);
    buf[cnt] = 0;
    os_eventq_put(&lines_queue, ev);

    if (console_compat_rx_cb) {
        console_compat_rx_cb();
    }
}

int
console_read(char *str, int cnt, int *newline)
{
    struct os_event *ev;
    struct console_input *cmd;
    size_t len;

    *newline = 0;
    ev = os_eventq_get_no_wait(&lines_queue);
    if (!ev) {
        return 0;
    }
    cmd = ev->ev_arg;
    len = strlen(cmd->line);
    if ((cnt - 1) < len) {
        len = cnt - 1;
    }

    if (len > 0) {
        memcpy(str, cmd->line, len);
        str[len] = '\0';
    } else {
        str[0] = cmd->line[0];
    }

    os_eventq_put(&avail_queue, ev);
    *newline = 1;
    return len;
}


int
console_init(console_rx_cb rx_cb)
{
    os_eventq_init(&lines_queue);
    os_eventq_init(&avail_queue);
    for (int i = 0; i < MAX_LINES_QUEUED; i++) {
        buf[i] = malloc(MAX_LINE_LENGTH);
        SYSINIT_PANIC_ASSERT(buf[i] != NULL);
        line_available_ev[i].ev_arg = &buf[i];
        os_eventq_put(&avail_queue, &line_available_ev[i]);
    }
    console_compat_rx_cb = rx_cb;
    return 0;
}

