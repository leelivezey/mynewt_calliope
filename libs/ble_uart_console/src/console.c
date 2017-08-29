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

static struct os_eventq lines_avail_queue;
static struct os_eventq lines_queue;

static struct os_eventq *shell_cmd_avail_queue;
static struct os_eventq *shell_cmd_queue;

static char* buf[MAX_LINES_QUEUED];
static struct os_event line_available_ev[MAX_LINES_QUEUED];

#define CONS_OUTPUT_MAX_LINE    60

/* Indicates whether the previous line of output was completed. */
//
//int console_is_midline;

static console_rx_cb console_compat_rx_cb; /* callback that input is ready */

/**
 * Prints the specified format string to the console.
 *
 * @return                      The number of characters that would have been
 *                                  printed if the console buffer were
 *                                  unlimited.  This return value is analogous
 *                                  to that of snprintf.
 */
int
console_printf(const char *fmt, ...)
{
    va_list args;
    char buf[CONS_OUTPUT_MAX_LINE];
    int num_chars;
    int len;

    num_chars = 0;

    /*
    if (console_get_ticks()) {
        // Prefix each line with a timestamp.
        if (!console_is_midline) {
            len = snprintf(buf, sizeof(buf), "%06lu ",
                           (unsigned long)os_time_get());
            num_chars += len;
            console_write(buf, len);
        }
    }
*/
    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    num_chars += len;
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1;
    }
    console_write(buf, len);
    va_end(args);

    return num_chars;
}


void console_write(const char *str, int cnt){
    static struct os_event *ev;
    static char *buf;

    ev = os_eventq_get_no_wait(&lines_avail_queue);
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

    os_eventq_put(&lines_avail_queue, ev);
    *newline = 1;
    return len;
}


int
console_init(console_rx_cb rx_cb)
{
    os_eventq_init(&lines_queue);
    os_eventq_init(&lines_avail_queue);
    for (int i = 0; i < MAX_LINES_QUEUED; i++) {
        buf[i] = malloc(MAX_LINE_LENGTH);
        SYSINIT_PANIC_ASSERT(buf[i] != NULL);
        line_available_ev[i].ev_arg = &buf[i];
        os_eventq_put(&lines_avail_queue, &line_available_ev[i]);
    }
    console_compat_rx_cb = rx_cb;
    return 0;
}

void console_set_queues(struct os_eventq *avail_queue,
                        struct os_eventq *cmd_queue){
    shell_cmd_avail_queue = avail_queue;
    shell_cmd_queue = cmd_queue;
}

void shell_cmd_write(const char *str, int cnt){
    static struct os_event *ev;
    static char *buf;

    ev = os_eventq_get_no_wait(shell_cmd_avail_queue);
    if (!ev) {
        return;
    }
    buf = ev->ev_arg;
    if( cnt > MAX_LINE_LENGTH-1) {
        cnt = MAX_LINE_LENGTH-1;
    }
    strncpy(buf, str, cnt);
    buf[cnt] = 0;
    os_eventq_put(shell_cmd_queue, ev);
}
