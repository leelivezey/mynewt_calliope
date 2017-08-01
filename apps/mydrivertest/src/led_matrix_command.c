//
// Created by Alfred Schilken on 29.05.17.
//

#include "syscfg/syscfg.h"
#include <shell/shell.h>
#include <console/console.h>
#include <assert.h>
#include <stdio.h>
#include <microbit_matrix/microbit_matrix.h>

static int matrix_shell_func(int argc, char **argv);

static struct shell_cmd matrix_cmd = {
        .sc_cmd = "matrix",
        .sc_cmd_func = matrix_shell_func,
};

void matrix_command_init(void) {
    int rc;
    rc = shell_cmd_register(&matrix_cmd);
    assert(rc == 0);
}

static int matrix_shell_func(int argc, char **argv) {
    if (argc < 2) {
        console_printf("matrix c <char> | p <string>\n");
        return 1;
    }
    int rc;

    if(strcmp(argv[1], "p") == 0){
        if (argc > 2) {
            char *argv2 = argv[2];
            rc = print_string(argv2, FALSE);
            console_printf("matrix: print string on LED matrix rc= %d\n", rc);
            return 0;
        }
    }

    if(strcmp(argv[1], "c") == 0){
        if (argc > 2) {
            char *argv2 = argv[2];
            rc = print_char(*argv2, FALSE);
            console_printf("matrix: print char on LED matrix rc= %d\n", rc);
            return 0;
        }
    }

    return 0;
}




