//
// Created by Alfred Schilken on 18.07.17.
//

#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <stdio.h>
#include <console/console.h>
#include <assert.h>
#include <sound/sound_pwm.h>

static int sound_shell_func(int argc, char **argv);

static struct shell_cmd sound_cmd = {
        .sc_cmd = "sound",
        .sc_cmd_func = sound_shell_func,
};

void sound_command_init(void) {
    int rc;
    rc = shell_cmd_register(&sound_cmd);
    assert(rc == 0);
}

static int sound_shell_func(int argc, char **argv) {
    if (argc != 2) {
        console_printf("usage: sound S|s|0|f  with 20 < f < 20000\n");
        return 1;
    }
    char* argv1 = argv[1];
    if (strlen(argv1) > 1 ) {
        int f;
        if(sscanf( argv1, "%d", &f) == 1) {
            sound_on((uint16_t)f);
            return 0;
        }
    }

    char color = argv1[0];
    if (color == '0') {
        sound_off();
        return 0;
    }
    if (color == 'S') {
        sound_silent(true);
        return 0;
    }
    if (color == 's') {
        sound_silent(false);
        return 0;
    }
    return 1;
}
