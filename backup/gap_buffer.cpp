//
// Created by darko on 03.3.21.
//

#include "gap_buffer.h"


const char SPECIAL_CHAR = (char) (0);
const int GAP_SIZE = 200;
const int BUFFER_SIZE = 5000;
char *buffer;
int gap_left;
int gap_right;
FILE *logFile;
struct editorConfig E;


void gapBufferGrow(int k, int position) {
    char a[BUFFER_SIZE - position - 1];
    for (int i = position; i < BUFFER_SIZE; i++) {
        /* a[i - position] = buffer[i]; */
        a[i - position] = *(buffer + i);
    }
    for (int i = position; i < k; i++) {
        /* buffer[i] = SPECIAL_CHAR; */
        *(buffer + i) = SPECIAL_CHAR;
    }
    for (int i = position + k; i < BUFFER_SIZE; i++) {
        /* buffer[i] = a[i - position - k]; */
        *(buffer + i) = a[i - position - k];
    }
}

void gapBufferLeft(int position) {
    while (position < gap_left) {
        gap_left--;
        gap_right--;
        *(buffer + gap_right + 1) = *(buffer + gap_left);
        *(buffer + gap_left) = SPECIAL_CHAR;
        /* buffer[gap_right + 1] = buffer[gap_left]; */
        /* buffer[gap_left] = SPECIAL_CHAR; */
    }
}

void gapBufferRight(int position) {
    while (position > gap_left) {
        gap_left++;
        gap_right++;
        /* buffer[gap_left - 1] = buffer[gap_right]; */
        /* buffer[gap_right] = SPECIAL_CHAR; */
        *(buffer + gap_left - 1) = *(buffer + gap_right);
        *(buffer + gap_right) = SPECIAL_CHAR;
    }
}

void gapBufferMoveCursor(int position) {
    if (position < gap_left) {
        gapBufferLeft(position);
    } else {
        gapBufferRight(position);
    }
}

void gapBufferInsert(char c, int position) {
    if (position != gap_left) {
        gapBufferMoveCursor(position);
    }
    if (gap_right == gap_left) {
        gapBufferGrow(200, position);
    }
    *(buffer + gap_left) = c;
    /* buffer[gap_left] = c; */
    gap_left++;
}

void gapBufferDelete(int position) {
    if (gap_left != position + 1) {
        gapBufferMoveCursor(position + 1);
    }
    *(buffer + position) = SPECIAL_CHAR;
    gap_left--;
}

void gapBufferInit() {
    buffer = (char *) (malloc(sizeof(char) * BUFFER_SIZE));
    if (buffer == NULL)
        die("malloc");
    int i;
    for (i = 0; i < BUFFER_SIZE; i++) {
        *(buffer + i) = SPECIAL_CHAR;
    }
    gap_left = 0;
    gap_right = GAP_SIZE - 1;
}

void gapBufferReadFile(char *fileName) {
    FILE *f = fopen(fileName, "r");
    int fd = fileno(f);
    char c;
    // GAP AT THE BEGINNING
    int position = GAP_SIZE;
    while (read(fd, &c, 1) == 1) {
        gapBufferInsert(c, position);
        position++;
    }
    gap_left = 0;
    gap_right = GAP_SIZE - 1;
}

void gapBufferFree() { free(buffer); }

