//
// Created by darko on 03.3.21.
//

#include "splay_tree.h"
#include "editor_config.h"
#include "utils.h"
#include <cairo.h>
#include <chrono>
#include <gdk/gdkrgba.h>
#include <gdk/gdkwindow.h>
#include <iostream>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <string>
#include <unistd.h>

void leftRotate(Node *x) {
    Node *y = x->right;
    if (y) {
        x->right = y->left;
        x->sizeRight = y->sizeLeft;
        if (y->left)
            y->left->parent = x;
        y->parent = x->parent;
    }

    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    if (y)
        y->left = x;
    x->parent = y;
    if (y) {
        y->sizeLeft += x->sizeLeft + x->piece->length;
    }
}

void rightRotate(Node *x) {
    Node *y = x->left;
    if (y) {
        x->left = y->right;
        x->sizeLeft = y->sizeRight;
        if (y->right)
            y->right->parent = x;
        y->parent = x->parent;
    }
    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    if (y)
        y->right = x;
    x->parent = y;
    if (y) {
        y->sizeRight += x->sizeRight + x->piece->length;
        //        x->sizeLeft -= y->piece->length;
    }
}

void splay(Node *x) {
    while (x->parent) {
        if (!x->parent->parent) {
            if (x->parent->left == x)
                rightRotate(x->parent);
            else
                leftRotate(x->parent);
        } else if (x->parent->left == x && x->parent->parent->left == x->parent) {
            rightRotate(x->parent->parent);
            rightRotate(x->parent);
        } else if (x->parent->right == x && x->parent->parent->right == x->parent) {
            leftRotate(x->parent->parent);
            leftRotate(x->parent);
        } else if (x->parent->left == x && x->parent->parent->right == x->parent) {
            rightRotate(x->parent);
            leftRotate(x->parent);
        } else {
            leftRotate(x->parent);
            rightRotate(x->parent);
        }
    }
}

void replace(struct Node *u, struct Node *v) {
    if (!u->parent)
        root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

struct Node *subtreeMinimum(struct Node *u, unsigned long lengthToRemove) {
    while (u->left) {
        u->sizeLeft -= lengthToRemove;
        u = u->left;
    }
    return u;
}

struct Node *subtreeMaximum(struct Node *u, unsigned long lengthToRemove) {
    while (u->right) {
        u->sizeRight -= lengthToRemove;
        u = u->right;
    }
    return u;
}

void textBufferInsert(std::string text, unsigned long length,
                      unsigned long idx) {

    // calculate time and compare
    unsigned long long now =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    if (now - E.bufferConfig.timeOfLastInter <= 1000 &&
        E.bufferConfig.interBuffer.text.size() < 40) {
        auto loopStart = E.bufferConfig.interBuffer.text.size();
        for (int i = loopStart; i < loopStart + length; i++) {
            if (text[i - loopStart] == '\n')
                E.bufferConfig.interBuffer.newLines.push_back(i);
        }
        E.bufferConfig.interBuffer.text += text;
        E.bufferConfig.timeOfLastInter = now;
        return;
    }
    // we need to add it to the tree and clean up the inter buffer
    if (E.bufferConfig.interBuffer.text.size() > 0) {
        _textBufferInsert(E.bufferConfig.interBuffer.text,
                          E.bufferConfig.interBuffer.text.size(),
                          E.bufferConfig.interStartIdx);
    }

    E.bufferConfig.interBuffer.text = text;
    E.bufferConfig.interBuffer.newLines.clear();
    E.bufferConfig.interStartIdx = idx;
    Piece newLeftOfInter(0, "");
    E.bufferConfig.leftOfInter = newLeftOfInter;

    E.bufferConfig.timeOfLastInter = now;
    for (int i = 0; i < text.size(); i++) {
        if (text[i] == '\n')
            E.bufferConfig.interBuffer.newLines.push_back(i);
    }
}

void _textBufferInsert(std::string text, unsigned long length,
                       unsigned long idx) {

    if (!root) {
        Node *newNode =
                new Node(new Piece(length, text), nullptr, nullptr, nullptr, 0, 0);
        root = newNode;
        return;
    }

    // insert char at idx
    Node *x = root;
    int seen = 0;
    while (true) {
        // idx < sizeLeft -> go left
        if (idx <= seen + x->sizeLeft) {
            if (x->left != nullptr) {
                x->sizeLeft += length;
                x = x->left;
                continue;
            } else {
                // put to the left
                Node *newNode =
                        new Node(new Piece(length, text), nullptr, nullptr, x, 0, 0);
                x->left = newNode;
                x->sizeLeft += length;
                splay(newNode);
                return;
            }
        }
        // idx >= sizeLeft + length
        if (idx >= seen + x->sizeLeft + x->piece->length) {
            if (x->right != nullptr) {
                seen += x->sizeLeft + x->piece->length;
                x->sizeRight += length;
                x = x->right;
                continue;
            } else {
                Node *newNode =
                        new Node(new Piece(length, text), nullptr, nullptr, x, 0, 0);
                x->right = newNode;
                x->sizeRight += length;
                splay(newNode);
                return;
            }
        }
        // idx in range (sizeLeft, sizeLeft + length) -> three new pieces
        if (idx >= seen + x->sizeLeft &&
            idx < seen + x->sizeLeft + x->piece->length) {
            struct Piece *currentPiece = x->piece;
            auto cutLeft = idx - seen - x->sizeLeft;

            // create left piece
            std::string leftPieceText = currentPiece->text.substr(0, cutLeft);
            auto leftPieceLength = cutLeft;
            Piece *leftPiece = new Piece(leftPieceLength, leftPieceText);

            // create right piece
            std::string rightPieceText =
                    currentPiece->text.substr(cutLeft, currentPiece->length - cutLeft);
            auto rightPieceLength = currentPiece->length - cutLeft;
            Piece *rightPiece = new Piece(rightPieceLength, rightPieceText);

            // modify existing piece
            delete currentPiece;
            currentPiece = new Piece(length, text);

            Node *leftNode = new Node(leftPiece, x->left, nullptr, x, x->sizeLeft, 0);
            Node *rightNode =
                    new Node(rightPiece, nullptr, x->right, x, 0, x->sizeRight);

            // existing node adjustments -> replace with new one
            x->sizeLeft = x->sizeLeft + leftPiece->length;
            x->sizeRight = x->sizeRight + rightPiece->length;
            x->piece = currentPiece;
            x->left = leftNode;
            x->right = rightNode;

            splay(x);
            return;
        }
    }
}

void textBufferRemove(int startIdx, int endIdx) {
    remove_(root, startIdx, endIdx, 0);
}

unsigned long remove_(Node *node, unsigned long startIdx, unsigned long endIdx,
                      unsigned long seen) {

    if (!node)
        return 0;

    auto subTreeStart = seen;
    auto subTreeEnd =
            seen + node->sizeLeft + node->piece->length + node->sizeRight - 1;
    if (endIdx < subTreeStart || startIdx > subTreeEnd) {
        // completely outside of subtree
        return 0;
    }

    // something inside needs to be removed
    if (startIdx >= subTreeStart || endIdx <= subTreeEnd) {

        auto pieceStart = seen + node->sizeLeft;
        auto pieceEnd = seen + node->sizeLeft + node->piece->length - 1;

        auto deletedLeft = remove_(node->left, startIdx, endIdx, seen);
        auto deletedRight = remove_(node->right, startIdx, endIdx,
                                    seen + node->sizeLeft + node->piece->length);
        node->sizeLeft -= deletedLeft;
        node->sizeRight -= deletedRight;

        if (pieceStart > endIdx || pieceEnd < startIdx)
            return deletedLeft + deletedRight;

        // delete whole piece
        if (pieceStart >= startIdx && pieceEnd <= endIdx) {

            auto deletedPieceLength = node->piece->length;
            if (!node->left && node->right) {
                // replace with right child
                if (node->parent) {
                    if (node->parent->left == node) {
                        node->parent->left = node->right;
                    } else {
                        node->parent->right = node->right;
                    }
                    node->right->parent = node->parent;
                } else {
                    root = node->right;
                    node->right->parent = nullptr;
                }
                delete node;
                node = nullptr;
                return deletedLeft + deletedRight + deletedPieceLength;
            }
            if (!node->right && node->left) {
                // replace with right child
                if (node->parent) {
                    if (node->parent->left == node) {
                        node->parent->left = node->left;
                    } else {
                        node->parent->right = node->left;
                    }
                    node->left->parent = node->parent;
                } else {
                    root = node->left;
                    node->left->parent = nullptr;
                }
                delete node;
                node = nullptr;
                return deletedLeft + deletedRight + deletedPieceLength;
            }

            // otherwise, dig inside to find replacement
            bool isLeft = false;
            Node *nodeToReplace = nullptr;
            if (node->left) {
                isLeft = true;
                nodeToReplace = subtreeMaximum(node->left, node->piece->length);
                Node *trav = node->left;
                node->sizeLeft -= nodeToReplace->piece->length;
            } else if (node->right) {
                nodeToReplace = subtreeMinimum(node->right, node->piece->length);
                node->sizeRight -= nodeToReplace->piece->length;
            }

            // delete whole node
            if (!nodeToReplace) {
                if (node->parent && node->parent->left == node)
                    node->parent->left = nullptr;
                else if (node->parent && node->parent->right == node)
                    node->parent->right = nullptr;
                else if (!node->parent) {
                    root = nullptr;
                }

                delete node;
                node = nullptr;
                return deletedLeft + deletedRight + deletedPieceLength;
            }


            // replace with candidate
            if (isLeft && nodeToReplace->left && nodeToReplace->parent != node) {
                nodeToReplace->left->parent = nodeToReplace->parent;
                nodeToReplace->parent->right = nodeToReplace->left;
            } else if (!isLeft && nodeToReplace->right &&
                       nodeToReplace->parent != node) {
                nodeToReplace->right->parent = nodeToReplace->parent;
                nodeToReplace->parent->left = nodeToReplace->right;
            }
            if (node->left && node->left != nodeToReplace) {
                nodeToReplace->left = node->left;
                if (node->left)
                    node->left->parent = nodeToReplace;
            }
            if (node->right && node->right != nodeToReplace) {
                if (node->right)
                    nodeToReplace->right = node->right;
                node->right->parent = nodeToReplace;
            }

            if (nodeToReplace->parent->left == nodeToReplace)
                nodeToReplace->parent->left = nullptr;
            else if (nodeToReplace->parent->right == nodeToReplace)
                nodeToReplace->parent->right = nullptr;

            if (node->parent) {
                if (node->parent->left && node->parent->left == node) {
                    node->parent->left = nodeToReplace;
                    nodeToReplace->parent = node->parent;

                } else if (node->parent->right && node->parent->right == node) {
                    node->parent->right = nodeToReplace;
                    nodeToReplace->parent = node->parent;
                }
            } else {
                nodeToReplace->parent = nullptr;
                root = nodeToReplace;
            }

            nodeToReplace->sizeLeft = node->sizeLeft;
            nodeToReplace->sizeRight = node->sizeRight;

            delete node;
            node = nullptr;
            return deletedLeft + deletedRight + deletedPieceLength;
        } else {
            // we need to cut out a slice of the piece
            auto oldPieceLength = node->piece->length;
            unsigned long firstSliceLength, secondSliceStart, secondSliceLength;
            firstSliceLength = 0;
            secondSliceLength = 0;

            if (startIdx > pieceStart) {
                firstSliceLength = startIdx - pieceStart;
            }
            if (endIdx < pieceEnd) {
                secondSliceStart = endIdx + 1 - pieceStart;
                secondSliceLength = pieceEnd - endIdx;
            }

            std::string slice;
            auto newTextLength = firstSliceLength + secondSliceLength;
            if (firstSliceLength) {
                slice = node->piece->text.substr(0, firstSliceLength);
            }
            if (secondSliceLength) {
                slice += node->piece->text.substr(secondSliceStart, secondSliceLength);
            }

            delete node->piece;
            node->piece = nullptr;

            node->piece = new Piece(newTextLength, slice);
            return deletedLeft + deletedRight + oldPieceLength - newTextLength;
        }
    }
}

void textBufferWrite(std::string text, std::vector<unsigned long> &newLines) {


    std::string textToDisplay = "";
    auto lastVisitedNewLineIndex = -1;
    auto currentRow = E.row;
    auto currentColumn = E.column;


    unsigned long long writeStartTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    auto newDisplayRow = E.displayRow;
    auto newDisplayColumn = E.displayColumn;

    std::vector<unsigned long> updatedNewLines;
    if (currentRow <= E.endRow) {
        for (auto newLine : newLines) {
            if (currentRow < E.startRow) {
                lastVisitedNewLineIndex = newLine;
                currentRow++;
                currentColumn = 1;
                continue;
            }
            if (currentRow > E.endRow) {
                break;
            }

            //now we are inside the valid rows range, check columns
            int currentSliceStart = lastVisitedNewLineIndex + 1;
            int currentSliceEnd = newLine - 1;
            int currentSliceOriginalLength = lastVisitedNewLineIndex > -1 ? newLine - lastVisitedNewLineIndex - 1 : newLine;


            if (E.startColumn > currentColumn) {
                currentSliceStart += E.startColumn - currentColumn;
            }
            if (E.endColumn < currentColumn + currentSliceOriginalLength - 1) {
                currentSliceEnd -= (currentColumn + currentSliceOriginalLength - 1) - E.endColumn;
            }

            if (currentSliceStart <= currentSliceEnd) {
                std::string processedSlice = text.substr(currentSliceStart, currentSliceEnd - currentSliceStart + 1);
                textToDisplay += processedSlice;
            }
            textToDisplay.append(1, '\n');

            newDisplayRow++;
            newDisplayColumn = 1;

            currentRow++;
            currentColumn = 1;

            updatedNewLines.push_back(textToDisplay.size() - 1);
            lastVisitedNewLineIndex = newLine;

        }
        if (currentRow >= E.startRow && currentRow <= E.endRow) {
            auto textEnd = text.size() - 1;
            auto currentSliceStart = lastVisitedNewLineIndex + 1;
            auto currentSliceEnd = textEnd;
            auto currentSliceOriginalLength = lastVisitedNewLineIndex > -1 ? textEnd - lastVisitedNewLineIndex - 1 : textEnd + 1;


            if (E.startColumn > currentColumn) {
                currentSliceStart += E.startColumn - currentColumn;
            }
            if (E.endColumn < currentColumn + currentSliceOriginalLength - 1) {
                currentSliceEnd -= (currentColumn + currentSliceOriginalLength - 1) - E.endColumn;
            }

            if (currentSliceEnd >= currentSliceStart && currentSliceStart < text.size() && currentSliceEnd >= 0) {
                std::string processedSlice = text.substr(currentSliceStart, currentSliceEnd - currentSliceStart + 1);
                textToDisplay += processedSlice;

                newDisplayColumn += processedSlice.size();
            }
        }
    }

    unsigned long long writeEndTime=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();


    std::pair<unsigned long, unsigned long> cursorPixelPosition = getCursorPixelPosition();
    //    unsigned long oldRow, oldColumn;
    //    oldRow = E.displayRow;
    //    oldColumn = E.displayColumn;

    if (updatedNewLines.size() > 0) {
        //do the newline hack
        cairoRegion = cairo_region_create();
        drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
        cr = gdk_drawing_context_get_cairo_context(drawingContext);
        cairo_set_source_rgba(cr, textColor.red, textColor.green, textColor.blue, 1.0);
        cairo_move_to(cr, cursorPixelPosition.first, cursorPixelPosition.second);

        std::string firstPart = textToDisplay.substr(0, updatedNewLines[0] + 1);
        pango_layout_set_text(layout, firstPart.c_str(), -1);
        pango_cairo_show_layout(cr, layout);

        gdk_window_end_draw_frame(window, drawingContext);
        cairo_region_destroy(cairoRegion);
    }
    if (updatedNewLines.size() == 0 || updatedNewLines[0] < textToDisplay.size() - 1) {

        cairoRegion = cairo_region_create();
        drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
        cr = gdk_drawing_context_get_cairo_context(drawingContext);
        cairo_set_source_rgba(cr, textColor.red, textColor.green, textColor.blue, 1.0);

        E.displayRow = updatedNewLines.size() == 0 ? E.displayRow : (E.displayRow + 1);
        E.displayColumn = updatedNewLines.size() == 0 ? E.displayColumn : 1;
        std::pair<unsigned long, unsigned long> cursorPixelPosition = getCursorPixelPosition();
        cairo_move_to(cr, cursorPixelPosition.first, cursorPixelPosition.second);

        unsigned long secondPartStart = updatedNewLines.size() == 0 ? 0 : (updatedNewLines[0] + 1);
        unsigned long secondPartLength = updatedNewLines.size() == 0 ? text.size() : (textToDisplay.size() - updatedNewLines[0] - 1);
        std::string secondPart = textToDisplay.substr(secondPartStart, secondPartLength);
        pango_layout_set_text(layout, secondPart.c_str(), -1);
        pango_cairo_show_layout(cr, layout);

        gdk_window_end_draw_frame(window, drawingContext);
        cairo_region_destroy(cairoRegion);
    }
    E.displayRow = newDisplayRow;
    E.displayColumn = newDisplayColumn;
}

void displayText(unsigned long startIdx, unsigned long endIdx) {

    unsigned long oldRow = E.row;
    unsigned long oldColumn = E.column;

    E.row = 1;
    E.column = 1;

    E.displayRow = 1;
    E.displayColumn = 1;

    //there is no inter, just traverse the whole tree
    if (E.bufferConfig.interBuffer.text.size() == 0) {
        E.n_rows = 1;
        E.n_words.clear();
        std::vector<unsigned long> v(1024, 0);
        E.n_words = v;
        traverse(root, 0, UINT32_MAX, 0);
    }
    // we need to traverse left
    else if (E.bufferConfig.leftOfInter.text.size() == 0 && E.bufferConfig.interStartIdx != 0) {
        // this will automatically fill up interLeft
        E.n_rows = 1;
        E.n_words.clear();
        std::vector<unsigned long> v(1024, 0);
        E.n_words = v;
        traverse(root, 0, E.bufferConfig.interStartIdx - 1, 0);
        E.bufferConfig.interBufferStartRow = E.row;
        E.bufferConfig.interBufferStartColumn = E.column;

        //process inter buffer
        textBufferWrite(E.bufferConfig.interBuffer.text, E.bufferConfig.interBuffer.newLines);
        unsigned long interBufferStartIdx = 0;
        auto interBufferEndIdx = E.bufferConfig.interBuffer.text.size() - 1;
        auto lastVisitedNewLineIndex = -1;

        for (unsigned long currentNewLineIndex :
             E.bufferConfig.interBuffer.newLines) {
            if (currentNewLineIndex < interBufferStartIdx ||
                currentNewLineIndex > interBufferEndIdx)
                continue;
            auto adjustedCurrentNewLineIndex =
                    currentNewLineIndex - interBufferStartIdx;
            E.column += adjustedCurrentNewLineIndex;
            if (lastVisitedNewLineIndex != -1) {
                E.column -= lastVisitedNewLineIndex + 1;
            }
            E.n_words[E.row] = E.column - 1;
            E.row++;
            E.column = 1;
            lastVisitedNewLineIndex = adjustedCurrentNewLineIndex;
        }
        E.column += E.bufferConfig.interBuffer.text.size();
        if (lastVisitedNewLineIndex != -1) {
            E.column -= lastVisitedNewLineIndex + 1;
        }
        E.n_words[E.row] = E.column - 1;

        traverse(root,
                 E.bufferConfig.interStartIdx,
                 UINT32_MAX, 0);
    } else {
        // we can just print interLeft, process interBuffer and then traverse right
        if (E.bufferConfig.leftOfInter.text.size() > 0) {
            textBufferWrite(E.bufferConfig.leftOfInter.text, E.bufferConfig.leftOfInter.newLines);
        }
        E.row = E.bufferConfig.interBufferStartRow;
        E.column = E.bufferConfig.interBufferStartColumn;


        // process interBuffer
        textBufferWrite(E.bufferConfig.interBuffer.text, E.bufferConfig.interBuffer.newLines);
        unsigned long interBufferStartIdx = 0;
        auto interBufferEndIdx = E.bufferConfig.interBuffer.text.size() - 1;
        auto lastVisitedNewLineIndex = -1;

        for (unsigned long currentNewLineIndex :
             E.bufferConfig.interBuffer.newLines) {
            if (currentNewLineIndex < interBufferStartIdx ||
                currentNewLineIndex > interBufferEndIdx)
                continue;
            auto adjustedCurrentNewLineIndex =
                    currentNewLineIndex - interBufferStartIdx;
            E.column += adjustedCurrentNewLineIndex;
            if (lastVisitedNewLineIndex != -1) {
                E.column -= lastVisitedNewLineIndex + 1;
            }
            E.n_words[E.row] = E.column - 1;
            E.row++;
            E.column = 1;
            lastVisitedNewLineIndex = adjustedCurrentNewLineIndex;
        }
        E.column += E.bufferConfig.interBuffer.text.size();
        if (lastVisitedNewLineIndex != -1) {
            E.column -= lastVisitedNewLineIndex + 1;
        }
        E.n_words[E.row] = E.column - 1;
        // traverse right
        traverse(root,
                 E.bufferConfig.interStartIdx,
                 UINT32_MAX, 0);
    }
    E.n_rows = E.row;
    editorMoveCursorToPosition(oldRow, oldColumn);
}

void traverse(Node *x, unsigned long startIdx, unsigned long endIdx,
              unsigned long seen) {
    if (!x)
        return;

    if (x->left) {
        traverse(x->left, startIdx, endIdx, seen);
    }
    auto pieceStartIdx = seen + x->sizeLeft;
    auto pieceEndIdx = seen + x->sizeLeft + x->piece->length - 1;
    // only process current node if not outside [startIdx, endIdx]
    if (startIdx <= pieceEndIdx && endIdx >= pieceStartIdx) {

        std::string currentPieceText = x->piece->text;
        std::vector<unsigned long> currentNewLines;
        // check if piece needs to be cut from the left
        if (startIdx > pieceStartIdx) {
            currentPieceText = currentPieceText.substr(startIdx - pieceStartIdx,
                                                       currentPieceText.size() -
                                                               startIdx + pieceStartIdx);
        }
        // check if piece needs to be cut from the right
        if (endIdx < pieceEndIdx) {
            currentPieceText = currentPieceText.substr(0, currentPieceText.size() -
                                                                  pieceEndIdx + endIdx);
        }


        auto adjustedStartIdx = std::max(int(startIdx) - int(pieceStartIdx), 0);
        auto adjustedEndIdx = adjustedStartIdx + currentPieceText.size() - 1;

        for (auto newLine : x->piece->newLines) {
            if (startIdx > pieceStartIdx && newLine < startIdx - pieceStartIdx)
                continue;
            if (endIdx < pieceEndIdx && newLine > adjustedEndIdx) {
                continue;
            }
            currentNewLines.push_back(newLine - adjustedStartIdx);
        }


        textBufferWrite(currentPieceText, currentNewLines);

        if (startIdx == 0 && endIdx == E.bufferConfig.interStartIdx) {
            E.bufferConfig.leftOfInter.text += currentPieceText;
            //add new lines to left of inter piece
            for (auto newLine : x->piece->newLines) {
                E.bufferConfig.leftOfInter.newLines.push_back(pieceStartIdx + newLine);
            }
        }

        auto lastVisitedNewLineIndex = -1;
        for (unsigned long currentNewLineIndex : x->piece->newLines) {
            if (currentNewLineIndex < adjustedStartIdx ||
                currentNewLineIndex > adjustedEndIdx)
                continue;
            auto adjustedCurrentNewLineIndex = currentNewLineIndex - adjustedStartIdx;
            E.column += adjustedCurrentNewLineIndex;
            if (lastVisitedNewLineIndex != -1) {
                E.column -= lastVisitedNewLineIndex + 1;
            }
            E.n_words[E.row] = E.column - 1;
            E.row++;
            E.column = 1;
            lastVisitedNewLineIndex = adjustedCurrentNewLineIndex;
        }
        E.column += currentPieceText.size();
        if (lastVisitedNewLineIndex != -1) {
            E.column -= lastVisitedNewLineIndex + 1;
        }
        E.n_words[E.row] = E.column - 1;

        editorMoveCursorToPosition(E.row, E.column);
    }
    // go through this node
    if (x->right) {
        traverse(x->right, startIdx, endIdx, seen + x->sizeLeft + x->piece->length);
    }
}


void textBufferFlushInterBuffer() {

    E.bufferConfig.timeOfLastInter = 0;
    if (E.bufferConfig.interBuffer.text.size() > 0) {
        _textBufferInsert(E.bufferConfig.interBuffer.text,
                          E.bufferConfig.interBuffer.text.size(),
                          E.bufferConfig.interStartIdx);

        Piece newInterBuffer(0, "");
        Piece newLeftOfInter(0, "");
        BufferConfig newBufferConfig;
        E.bufferConfig = newBufferConfig;
    }
}

void readFile(std::string &fileName) {
    GFile *file = g_file_new_for_commandline_arg(fileName.c_str());
    GFileInputStream *fin = g_file_read(file, NULL, NULL);

    char buffer[1024];
    unsigned long alreadyRead = 0;
    while (TRUE) {

        unsigned long dataSize = g_input_stream_read(G_INPUT_STREAM(fin), buffer, 1024, NULL, NULL);
        if (dataSize == 0) {
            break;
        }
        std::string s(buffer, buffer + dataSize);
        _textBufferInsert(s, dataSize, alreadyRead);
        alreadyRead += dataSize;
    }
    g_object_unref(fin);
    g_object_unref(file);
}