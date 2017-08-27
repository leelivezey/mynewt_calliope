//
// Created by Alfred Schilken on 27.08.17.
//
#include "sysinit/sysinit.h"
#include "init_fcb_log.h"
#include "fcb/fcb.h"
#include "log/log.h"
#include "sysflash/sysflash.h"

#if MYNEWT_VAL(LOG_FCB)

struct fcb_log the_log_fcb;
static struct flash_area sector;

/**
 * Reboot log initilization
 * @param type of log(console or storage); number of entries to restore
 * @return 0 on success; non-zero on failure
 */
int
init_log_fcb_handler(void) {
    const struct flash_area *ptr;
    struct fcb *fcbp = &the_log_fcb.fl_fcb;
    int rc = flash_area_open(FLASH_AREA_FCB_LOG, &ptr);
    if (rc) {
        return rc;
    }
    fcbp = &the_log_fcb.fl_fcb;
    sector = *ptr;
    fcbp->f_sectors = &sector;
    fcbp->f_sector_cnt = 1;
    fcbp->f_magic = 0x7EADBADF;
    fcbp->f_version = g_log_info.li_version;

    rc = fcb_init(fcbp);
    if (rc) {
        flash_area_erase(ptr, 0, ptr->fa_size);
        rc = fcb_init(fcbp);
    }
    return rc;
}
#endif

