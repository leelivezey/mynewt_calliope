#include "sysinit/sysinit.h"
#include "os/os.h"
#include "console/console.h"
#include "config/config.h"
#include "host/ble_hs.h"
#include "log/log.h"
#include <reboot/log_reboot.h>
#include "eddystone_util.h"
#include "init_fcb_log.h"

#if MYNEWT_VAL(LOG_CONSOLE) || MYNEWT_VAL(LOG_FCB)
struct log ibeacon_log;
#if MYNEWT_VAL(LOG_CONSOLE)
#define IBEACON_LOG_MODULE  (LOG_MODULE_PERUSER + 0)
#define IBEACON_LOG(lvl, ...) LOG_ ## lvl(&ibeacon_log, IBEACON_LOG_MODULE, __VA_ARGS__)
#endif

#if MYNEWT_VAL(LOG_FCB)
// remove the "static" in sys/reboot/src/log_reboot.c to make it public
extern struct log reboot_log;
#define IBEACON_LOG_MODULE  (LOG_MODULE_PERUSER + 0)
#define IBEACON_LOG(lvl, ...) LOG_ ## lvl(&ibeacon_log, IBEACON_LOG_MODULE, __VA_ARGS__)
#endif
#else
#define IBEACON_LOG(__l, __mod, ...) IGNORE(__VA_ARGS__)
#endif

bool beacon_is_running = false;

static void update_adv();

static char *ibeacon_conf_get(int argc, char **argv, char *val, int val_len_max);

static int ibeacon_conf_set(int argc, char **argv, char *val);

static int ibeacon_conf_export(void (*export_func)(char *name, char *val), enum conf_export_tgt tgt);

struct conf_handler ibeacon_conf_handler = {
        .ch_name = "ibeacon",
        .ch_get = ibeacon_conf_get,
        .ch_set = ibeacon_conf_set,
        .ch_export = ibeacon_conf_export,
        .ch_commit = NULL
};


static uint16_t major = MYNEWT_VAL(IBEACON_MAJOR);
static uint16_t minor = MYNEWT_VAL(IBEACON_MINOR);
static char minor_str[6];
static char major_str[6];
static char eddystone_full_url[BLE_EDDYSTONE_URL_MAX_LEN+12+6];
static void
init_config() {
    int rc = conf_register(&ibeacon_conf_handler);
    assert(rc == 0);
    conf_load();
    reboot_start(hal_reset_cause());
}

static char *
ibeacon_conf_get(int argc, char **argv, char *val, int val_len_max) {
    if (argc == 1) {
        if (!strcmp(argv[0], "minor")) {
            return conf_str_from_value(CONF_INT16, &minor,
                                       minor_str, sizeof minor_str);
        } else if (!strcmp(argv[0], "major")) {
            return conf_str_from_value(CONF_INT16, &major,
                                       major_str, sizeof major_str);
        } else if (!strcmp(argv[0], "url")) {
            return eddystone_full_url;
        }
    }
    return NULL;
}

static int
ibeacon_conf_set(int argc, char **argv, char *val) {
    if (argc == 1) {
        if (!strcmp(argv[0], "minor")) {
            int rc = CONF_VALUE_SET(val, CONF_INT16, minor);
            IBEACON_LOG(INFO, "in ibeacon_conf_set, minor=%d\n", minor);
            eddystone_full_url[0] = 0;
            if (beacon_is_running) {
                update_adv();
            }
            return rc;
        } else if (!strcmp(argv[0], "major")) {
            return CONF_VALUE_SET(val, CONF_INT16, major);
        } else if (!strcmp(argv[0], "url")) {
            uint8_t not_used;
            int rc = cmd_parse_eddystone_url(val, &not_used,
                                             eddystone_full_url, // only temporary
                                             &not_used,
                                             &not_used);
            if (rc != 0) {
                eddystone_full_url[0] = 0;
                return OS_INVALID_PARM; // too long or invalid
            }
            rc = CONF_VALUE_SET(val, CONF_STRING, eddystone_full_url);
            if (beacon_is_running) {
                update_adv();
            }
            return rc;
        }
    }
    return OS_ENOENT;
}

static int
ibeacon_conf_export(void (*export_func)(char *name, char *val),
                    enum conf_export_tgt tgt) {
//    if (tgt == CONF_EXPORT_SHOW) {
//    }
// for CONF_EXPORT_SHOW and CONF_EXPORT_PERSIST
    export_func("ibeacon/minor", conf_str_from_value(CONF_INT16, &minor,
                                                     minor_str, sizeof minor_str));
    export_func("ibeacon/major", conf_str_from_value(CONF_INT16, &major,
                                                     major_str, sizeof major_str));
    export_func("ibeacon/url", (char *) eddystone_full_url);
    return 0;
}


static void
ble_app_set_addr(void) {
    ble_addr_t addr;
    int rc;

    rc = ble_hs_id_gen_rnd(1, &addr);
    assert(rc == 0);

    rc = ble_hs_id_set_rnd(addr.val);
    assert(rc == 0);
}


static void
ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    uint8_t uuid128[16];
    int rc;

    /* Arbitrarily set the UUID to a string of 0x11 bytes. */
    memset(uuid128, 0x11, sizeof uuid128);

    if (strlen(eddystone_full_url) > 0) {
        uint8_t eddystone_url_scheme;
        uint8_t eddystone_url_body_len;
        uint8_t eddystone_url_suffix;
        char eddystone_url_body[BLE_EDDYSTONE_URL_MAX_LEN];
        cmd_parse_eddystone_url(eddystone_full_url, &eddystone_url_scheme,
                                         eddystone_url_body,
                                         &eddystone_url_body_len,
                                         &eddystone_url_suffix);
        fields = (struct ble_hs_adv_fields){ 0 };
        rc = ble_eddystone_set_adv_data_url(&fields,
                                            eddystone_url_scheme,
                                            eddystone_url_body,
                                            eddystone_url_body_len,
                                            eddystone_url_suffix);
    } else {
        /* Major version=2; minor version=10. */
        rc = ble_ibeacon_set_adv_data(uuid128, major, minor);
        assert(rc == 0);
    }

    /* Begin advertising. */
    adv_params = (struct ble_gap_adv_params) {0};
    rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
                           &adv_params, NULL, NULL);
    assert(rc == 0);
    IBEACON_LOG(INFO, "in ble_app_advertise, major=%d, minor=%d\n", major, minor);
    beacon_is_running = true;
}

static void
update_adv() {
    console_printf("in update_adv\n");
    IBEACON_LOG(INFO, "in update_adv, minor=%d\n", minor);
    ble_gap_adv_stop();
    ble_app_advertise();
}

static void
ble_app_on_sync(void) {

    /* Generate a non-resolvable private address. */
    ble_app_set_addr();

    /* Advertise indefinitely. */
    ble_app_advertise();
}

int
main(int argc, char **argv) {
    sysinit();
#if MYNEWT_VAL(LOG_CONSOLE)
    log_register("ibeacon", &ibeacon_log, &log_console_handler, NULL, LOG_SYSLEVEL);
#endif

#if MYNEWT_VAL(LOG_FCB)
    init_log_fcb_handler();
    log_register("ibeacon", &ibeacon_log, &log_fcb_handler, &the_log_fcb, LOG_SYSLEVEL);
#endif
    IBEACON_LOG(INFO, "logging active \n");
    init_config();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    /* As the last thing, process events from default event queue. */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
}