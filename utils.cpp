//
// Created by darko on 04.3.21.
//

#include "utils.h"
#include <gdk/gdkdrawingcontext.h>

Node *root;
struct editorConfig E;
unsigned long TEXT_START_X = 0;
unsigned long TEXT_START_Y = 0;

unsigned long APPROX_CHAR_WIDTH = 0;
unsigned long APPROX_CHAR_HEIGHT = 0;

GdkWindow *window;
PangoContext *context;
PangoLayout *layout;
GdkDrawingContext *drawingContext;
cairo_region_t *cairoRegion;
cairo_t *cr;

GdkRGBA backgroundColor;
GdkRGBA textColor;
GdkRGBA caretColor;

int LAYOUT_WIDTH, LAYOUT_HEIGHT;

void printTree(std::string message)
{
    g_print("%s\n", message.c_str());
    printNode(root);
}

void printNode(Node *x)
{
    if (!x)
        return;
    if (x->left)
    {
        g_print("GOING LEFT\n");
        printNode(x->left);
    }
    g_print("%s%s%s%d%s%d%s", "NODE WITH TEXT: ", x->piece->text.c_str(), " szLeft: ", x->sizeLeft,
            " szRifht: ", x->sizeRight, " NEWLINES AT: ");
    for (int i = 0; i < x->piece->newLines.size(); i++)
    {
        g_print("%d ", x->piece->newLines[i]);
    }
    g_print("\n");
    if (x->right)
    {
        g_print("GOING RIGHT\n");
        printNode(x->right);
    }
}

std::pair<unsigned long, unsigned long> getCursorPixelPosition()
{

    int pixelPositionX = TEXT_START_X + (E.displayColumn - 1) * APPROX_CHAR_WIDTH;
    int pixelPositionY = TEXT_START_Y + (E.displayRow - 1) * APPROX_CHAR_HEIGHT;
    return std::make_pair(pixelPositionX, pixelPositionY);
}

std::pair<unsigned long, unsigned long> getCaretCursorPixelPosition()
{

    int pixelPositionX = TEXT_START_X + (E.column - E.startColumn) * APPROX_CHAR_WIDTH;
    int pixelPositionY = TEXT_START_Y + (E.row - E.startRow) * APPROX_CHAR_HEIGHT;
    return std::make_pair(pixelPositionX, pixelPositionY);
}

std::pair<unsigned long, unsigned long> getRowColumnDataForCoords(unsigned long xRaw, unsigned long yRaw)
{

    unsigned long column = (xRaw - TEXT_START_X) / APPROX_CHAR_WIDTH + E.startColumn;
    unsigned long row = (yRaw - TEXT_START_Y) / APPROX_CHAR_HEIGHT + E.startRow;

    return std::make_pair(row, column);
}