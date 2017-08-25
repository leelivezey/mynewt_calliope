//
// Created by Alfred Schilken on 25.08.17.
//

#ifndef MYPROJ_EDDYSTONE_UTIL_H
#define MYPROJ_EDDYSTONE_UTIL_H

#include "host/ble_eddystone.h"
#include "os/os.h"

extern int
cmd_parse_eddystone_url(char *full_url, uint8_t *out_scheme, char *out_body,
                        uint8_t *out_body_len, uint8_t *out_suffix);

#endif //MYPROJ_EDDYSTONE_UTIL_H
