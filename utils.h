//
// Created by darko on 04.3.21.
//

#ifndef DBEDIT_UTILS_H
#define DBEDIT_UTILS_H

#include <cstdio>
#include <fstream>
#include <gdk/gdk.h>
#include <string>
#include <vector>

typedef unsigned long ul;
struct Piece
{
    ul length;
    std::string text;
    std::vector<ul> newLines;

    Piece(ul length_, std::string text_)
    {
        length = length_;
        text = text_;

        std::vector<ul> newLines_;
        for (int i = 0; i < length; i++)
        {
            if (text[i] == '\n')
                newLines_.push_back(i);
        }
        newLines = newLines_;
    }
};

struct Node
{
    Piece *piece;
    Node *left, *right, *parent;
    ul sizeLeft, sizeRight;
    Node(Piece *p, Node *l, Node *r, Node *parent_, ul szLeft, ul szRight)
    {
        piece = p;
        left = l;
        right = r;
        parent = parent_;
        sizeLeft = szLeft;
        sizeRight = szRight;
    }
    ~Node()
    {
        if (piece)
            delete piece;

        left = nullptr;
        right = nullptr;
        piece = nullptr;
        parent = nullptr;
    }
};
struct BufferConfig
{
    unsigned long long timeOfLastInter;
    unsigned long interStartIdx;
    unsigned long interBufferStartRow;
    unsigned long interBufferStartColumn;
    Piece interBuffer = Piece(0, "");
    Piece leftOfInter = Piece(0, "");

    BufferConfig()
    {
        timeOfLastInter = UINT32_MAX;
        interStartIdx = 0;
        interBufferStartRow = 1;
        interBufferStartColumn = 1;
    };
};

struct editorConfig
{
    unsigned long startRow, endRow;
    unsigned long startColumn, endColumn;
    unsigned long row, column;
    unsigned long displayRow, displayColumn;
    unsigned long n_rows;
    std::vector<unsigned long> n_words;
    BufferConfig bufferConfig;
};

extern Node *root;
extern struct editorConfig E;
extern unsigned long TEXT_START_X;
extern unsigned long TEXT_START_Y;

extern unsigned long APPROX_CHAR_WIDTH;
extern unsigned long APPROX_CHAR_HEIGHT;

extern GdkRGBA backgroundColor;
extern GdkRGBA textColor;
extern GdkRGBA caretColor;

extern int LAYOUT_WIDTH, LAYOUT_HEIGHT;

extern GdkWindow *window;
extern PangoContext *context;
extern PangoLayout *layout;
extern GdkDrawingContext *drawingContext;
extern cairo_region_t *cairoRegion;
extern cairo_t *cr;

void printTree(std::string message);
void printNode(Node *x);
std::pair<unsigned long, unsigned long> getCursorPixelPosition();
std::pair<unsigned long, unsigned long> getCaretCursorPixelPosition();
std::pair<unsigned long, unsigned long> getRowColumnDataForCoords(unsigned long xRaw, unsigned long yRaw);

#endif // DBEDIT_UTILS_H
