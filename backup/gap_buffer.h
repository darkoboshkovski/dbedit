//
// Created by darko on 03.3.21.
//

#ifndef DBEDIT_GAP_BUFFER_H
#define DBEDIT_GAP_BUFFER_H


#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../editor_config.h"

extern const char SPECIAL_CHAR;
extern const int GAP_SIZE;
extern const int BUFFER_SIZE;
extern char *buffer;
extern int gap_left;
extern int gap_right;

void die(const char *s);

void gapBufferGrow(int k, int position);

void gapBufferLeft(int position);

void gapBufferRight(int position);

void gapBufferMoveCursor(int position);

void gapBufferInsert(char c, int position);

void gapBufferDelete(int position);

void gapBufferInit();

void gapBufferReadFile(char *fileName);

void gapBufferFree();


#endif //DBEDIT_GAP_BUFFER_H
