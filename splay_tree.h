//
// Created by darko on 03.3.21.
//

#ifndef DBEDIT_SPLAY_TREE_H
#define DBEDIT_SPLAY_TREE_H

#include <string>
#include "utils.h"

void leftRotate(Node *x);

void rightRotate(Node *x);

void splay(Node *x);

void replace(Node *u, Node *v);

Node *subtreeMaximum(Node *x, unsigned long lengthToRemove);

Node *subtreeMinimum(Node *x, unsigned long lengthToRemove);

void textBufferInsert(std::string text, unsigned long length, unsigned long idx);

void textBufferRemove(int startIdx, int endIdx);

unsigned long remove_(struct Node *x, unsigned long startIdx, unsigned long endIdx, unsigned long seen);

void displayText(unsigned long startIdx, unsigned long endIdx);

void traverse(Node *x, unsigned long startIdx, unsigned long endIdx, unsigned long seen);

void readFile(std::string &fileName);

#endif //DBEDIT_SPLAY_TREE_H
