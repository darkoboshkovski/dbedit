//
// Created by darko on 04.3.21.
//

#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

Node *root;
struct editorConfig E;
std::ofstream logFile;

// so bad
void die(char *message) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(message);
  logFile.close();
  E.n_words.clear();
  dieNode(root);
  exit(1);
}

void dieNode(Node *x) {
  if (!x) {
    return;
  }
  if (x->left)
    dieNode(x->left);
  if (x->right)
    dieNode(x->right);
  //    free(x->piece->text);
  free(x->piece);
  free(x);
}

void printTree(std::string message) {
  logFile << message << std::endl;
  if (root && root->parent) {
    logFile << "OMG ROOT HAS A PARENT" << std::endl;
  }
  printNode(root);
}

void printNode(Node *x) {
  if (!x)
    return;
  if (x->left) {
    logFile << "GOING LEFT" << std::endl;
    printNode(x->left);
  }
  logFile << "NODE WITH TEXT: "<< x->piece->text << " szLeft: " << x->sizeLeft
          << " szRight: "
          << x->sizeRight
          << " NEWLINES AT: ";
  for (int i = 0; i < x->piece->newLines.size(); i++) {
    logFile << x->piece->newLines[i] << " ";
  }
  logFile << std::endl;
  if (x->right) {
    logFile << "GOING RIGHT" << std::endl;
    printNode(x->right);
  }
}