//
// Created by darko on 03.3.21.
//

#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <malloc.h>
#include "editor_config.h"

void editorMoveCursorToPosition(unsigned long x, unsigned long y) {
    char x_str[8];
    char y_str[8];
    if (sprintf(x_str, "%d", (int)x) < 0)
        die("sprintf");
    if (sprintf(y_str, "%d", (int)y) < 0)
        die("sprintf");
    auto len_x = (size_t)(strlen(x_str));
    auto len_y = (size_t)(strlen(y_str));
    auto command_size = (size_t)(4 + len_x + len_y);
    char command[command_size];
    if (snprintf(command, command_size + 1, "%s%s%s%s%s", "\x1b[", x_str, ";",
                 y_str, "H") < 0)
        die("snprintf");
    write(STDOUT_FILENO, command, command_size);
    E.row = x;
    E.column = y;
}
