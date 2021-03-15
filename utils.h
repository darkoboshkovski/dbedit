//
// Created by darko on 04.3.21.
//

#ifndef DBEDIT_UTILS_H
#define DBEDIT_UTILS_H

#include <cstdio>
#include <fstream>
#include <string>
#include <termios.h>
#include <vector>

typedef unsigned long ul;
struct Piece {
  ul length;
  std::string text;
  std::vector<ul> newLines;
  Piece(ul length_, std::string text_) {
    length = length_;
    text = text_;

    std::vector<ul> newLines_;
    for (int i = 0; i < length; i++) {
      if (text[i] == '\n')
        newLines_.push_back(i);
    }
    newLines = newLines_;
  }
  // NOTE no destructor for now
};

struct Node {
  Piece *piece;
  Node *left, *right, *parent;
  ul sizeLeft, sizeRight;
  Node(Piece *p, Node *l, Node *r, Node *parent_, ul szLeft, ul szRight) {
    piece = p;
    left = l;
    right = r;
    parent = parent_;
    sizeLeft = szLeft;
    sizeRight = szRight;
  }
  ~Node() {
    if (piece)
      delete piece;

    left = nullptr;
    right = nullptr;
    piece = nullptr;
    // Watch out
    parent = nullptr;
  }
};

struct editorConfig {
  unsigned long screenrows;
  unsigned long screencols;
  unsigned long row, column;
  unsigned long n_rows;
  std::vector <unsigned long> n_words;
  struct termios orig_termios;
  unsigned long control_char;
};
extern Node *root;
extern struct editorConfig E;
extern std::ofstream logFile;

void die(char *message);
void dieNode(Node *x);
void printTree(std::string message);
void printNode(Node *x);

#endif // DBEDIT_UTILS_H
