//
// Created by Alfred Schilken on 18.07.17.
//

#include <shell/shell.h>
#include <stdio.h>
#include <console/console.h>
#include <assert.h>
#include <ws2812b/ws2812b.h>

static int rgb_shell_func(int argc, char **argv);

static struct shell_cmd rgb_cmd = {
        .sc_cmd = "rgb",
        .sc_cmd_func = rgb_shell_func,
};

void rgb_command_init(void) {
    int rc;
    rc = shell_cmd_register(&rgb_cmd);
    ws2812_init();
    assert(rc == 0);
}

static int rgb_shell_func(int argc, char **argv) {
    if (argc != 2) {
        console_printf("usage: rgb r|g|b\n");
        return 1;
    }
    char* argv1 = argv[1];
    if (strlen(argv1) == 6) {
        int r, g, b;
        if(sscanf( argv1, "%02x%02x%02x", &r, &g, &b) == 3) {
            rgb_set((uint8_t) r, (uint8_t) g, (uint8_t) b);
            rgb_send();
            return 0;
        }
    }

    char color = argv1[0];
    if (color == '0') {
        uint8_t zero = 0;
        rgb_set((uint8_t)zero, (uint8_t)zero, (uint8_t)zero);
        rgb_send();
        return 0;
    }
    if (color == 'r') {
        rgb_set((uint8_t)64, (uint8_t)0, (uint8_t)0);
        rgb_send();
        return 0;
    }
    if (color == 'g') {
        rgb_set((uint8_t)0, (uint8_t)64,(uint8_t)0);
        rgb_send();
        return 0;
    }
    if (color == 'b') {
        rgb_set((uint8_t)0, (uint8_t)0, (uint8_t)64);
        rgb_send();
        return 0;
    }
    return 1;
}
