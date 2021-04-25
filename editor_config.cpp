//
// Created by darko on 03.3.21.
//

#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <malloc.h>
#include "editor_config.h"


void editorMoveCursorToPosition(unsigned long x, unsigned long y) {
    E.row = x;
    E.column = y;
}