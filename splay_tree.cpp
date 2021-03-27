//
// Created by darko on 03.3.21.
//

#include "splay_tree.h"
#include "editor_config.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>

void leftRotate(Node *x) {
  printTree("BEFORE LEFT ROTATE");
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
  printTree("AFTER LEFT ROTATE");
}

void rightRotate(Node *x) {
  printTree("BEFORE RIGHT ROTATE");
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
  printTree("AFTER RIGHT ROTATE");
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
      E.bufferConfig.interBuffer.size() < 40) {
    auto loopStart = E.bufferConfig.interBuffer.size();
    for (int i = loopStart; i < loopStart + length; i++) {
      if (text[i - loopStart] == '\n')
        E.bufferConfig.interBufferNewlines.push_back(i);
    }
    E.bufferConfig.interBuffer += text;
    E.bufferConfig.timeOfLastInter = now;
    return;
  }
  // we need to add it to the tree and clean up the inter buffer
  if (E.bufferConfig.interBuffer.size() > 0) {
    logFile << "VLEGOV TUKA FOR SOME REASON" <<std::endl;
    _textBufferInsert(E.bufferConfig.interBuffer,
                      E.bufferConfig.interBuffer.size(),
                      E.bufferConfig.interStartIdx);
  }

  E.bufferConfig.interBuffer = text;
  E.bufferConfig.interBufferNewlines.clear();
  E.bufferConfig.interStartIdx = idx;
  E.bufferConfig.leftOfInter = "";
  E.bufferConfig.timeOfLastInter = now;
  for (int i = 0; i < text.size(); i++) {
    if (text[i] == '\n')
      E.bufferConfig.interBufferNewlines.push_back(i);
  }
}

void _textBufferInsert(std::string text, unsigned long length,
                       unsigned long idx) {

  logFile << "STARTING INSERT OF TEXT " << text << " AT " << idx << std::endl;
  if (!root) {
    logFile << "INSIDE IF WHEN NO ROOT INSERT" << std::endl;
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
  logFile << "STARTING REMOVE FROM " << startIdx << " TO " << endIdx
          << std::endl;
  printTree("TREE BEFORE REMOVE");
  remove_(root, startIdx, endIdx, 0);
  logFile << "FINISHED REMOVE FROM " << startIdx << " TO " << endIdx
          << std::endl;
  printTree("TREE AFTER REMOVE");
}

unsigned long remove_(Node *node, unsigned long startIdx, unsigned long endIdx,
                      unsigned long seen) {

  logFile << "YO" << std::endl;
  if (!node)
    return 0;

  auto subTreeStart = seen;
  auto subTreeEnd =
      seen + node->sizeLeft + node->piece->length + node->sizeRight - 1;
  logFile << "RECURSIVE FOR SUBTREE " << subTreeStart << " " << subTreeEnd
          << std::endl;
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
    logFile << "DEL LEFT RIGHT " << deletedLeft << " " << deletedRight
            << std::endl;
    node->sizeLeft -= deletedLeft;
    node->sizeRight -= deletedRight;

    if (pieceStart > endIdx || pieceEnd < startIdx)
      return deletedLeft + deletedRight;

    logFile << "BEFORE IF" << std::endl;
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
        logFile << "REPLACING WITH RIGHT CHILD" << std::endl;
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
        logFile << "NO NODE TO REPLACE" << std::endl;
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

      logFile << "BEFORE SWITCHING NODE WITH " << nodeToReplace->piece->text
              << std::endl;

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
      logFile << "STARTING SLICING" << std::endl;
      auto oldPieceLength = node->piece->length;
      logFile << "OLD PIECE LENGTH " << oldPieceLength << std::endl;
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
        logFile << "FIRST SLICE LEN " << firstSliceLength << std::endl;
        slice = node->piece->text.substr(0, firstSliceLength);
      }
      if (secondSliceLength) {
        logFile << "SECOND SLICE LEN " << secondSliceLength << std::endl;
        slice += node->piece->text.substr(secondSliceStart, secondSliceLength);
      }

      logFile << "NEW SLICE: " << slice << std::endl;
      delete node->piece;
      node->piece = nullptr;

      node->piece = new Piece(newTextLength, slice);
      return deletedLeft + deletedRight + oldPieceLength - newTextLength;
    }
  }
}

void displayText(unsigned long startIdx, unsigned long endIdx) {

  unsigned long oldRow = E.row;
  unsigned long oldColumn = E.column;

  E.row = 1;
  E.column = 1;

  //there is no inter, just traverse the whole tree
  if (E.bufferConfig.interBuffer.size() == 0) {
    E.n_rows = 1;
    E.n_words.clear();
    std::vector<unsigned long> v(1024, 0);
    E.n_words = v;
    traverse(root, 0, UINT32_MAX, 0);

  }
  // we need to traverse left
  else if (E.bufferConfig.leftOfInter.size() == 0 && E.bufferConfig.interStartIdx != 0) {
    // this will automatically fill up interLeft
    E.n_rows = 1;
    E.n_words.clear();
    std::vector<unsigned long> v(1024, 0);
    E.n_words = v;
    traverse(root, 0, E.bufferConfig.interStartIdx - 1, 0);
    E.bufferConfig.interBufferStartRow = E.row;
    E.bufferConfig.interBufferStartColumn = E.column;

    // process interBuffer
    write(STDOUT_FILENO, E.bufferConfig.interBuffer.c_str(),
          E.bufferConfig.interBuffer.size());
    unsigned long interBufferStartIdx = 0;
    auto interBufferEndIdx = E.bufferConfig.interBuffer.size() - 1;
    auto lastVisitedNewLineIndex = -1;

    for (unsigned long currentNewLineIndex :
         E.bufferConfig.interBufferNewlines) {
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
    E.column += E.bufferConfig.interBuffer.size();
    if (lastVisitedNewLineIndex != -1) {
      E.column -= lastVisitedNewLineIndex + 1;
    }
    E.n_words[E.row] = E.column - 1;

    traverse(root,
             E.bufferConfig.interStartIdx,
             UINT32_MAX, 0);
  } else {
    // we can just print interLeft, process interBuffer and then traverse right
    if (E.bufferConfig.leftOfInter.size() > 0) {
      write(STDOUT_FILENO, E.bufferConfig.leftOfInter.c_str(),
            E.bufferConfig.leftOfInter.size());
    }
    E.row = E.bufferConfig.interBufferStartRow;
    E.column = E.bufferConfig.interBufferStartColumn;


    // process interBuffer
    write(STDOUT_FILENO, E.bufferConfig.interBuffer.c_str(),
          E.bufferConfig.interBuffer.size());
    unsigned long interBufferStartIdx = 0;
    auto interBufferEndIdx = E.bufferConfig.interBuffer.size() - 1;
    auto lastVisitedNewLineIndex = -1;

    for (unsigned long currentNewLineIndex :
         E.bufferConfig.interBufferNewlines) {
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
    E.column += E.bufferConfig.interBuffer.size();
    if (lastVisitedNewLineIndex != -1) {
      E.column -= lastVisitedNewLineIndex + 1;
    }
    E.n_words[E.row] = E.column - 1;
    // traverse right
    traverse(root,
             E.bufferConfig.interStartIdx + E.bufferConfig.interBuffer.size(),
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

    write(STDOUT_FILENO, currentPieceText.c_str(), currentPieceText.size());
    if (startIdx == 0 && endIdx == E.bufferConfig.interStartIdx) {
      E.bufferConfig.leftOfInter += currentPieceText;
    }
    auto adjustedStartIdx = std::max(int(startIdx) - int(pieceStartIdx), 0);
    auto adjustedEndIdx = adjustedStartIdx + currentPieceText.size() - 1;
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
    //  logFile << "AFTER PRINTING ROWS " << E.row <<std::endl;
    //  for (int i = 1 ; i<=E.row; i++) {
    //    logFile << " ROW " << i << " HAS " <<E.n_words[i] << std::endl;
    //  }

    editorMoveCursorToPosition(E.row, E.column);
  }
  // go through this node
  if (x->right) {
    traverse(x->right, startIdx, endIdx, seen + x->sizeLeft + x->piece->length);
  }
}

void readFile(std::string &fileName) {
  std::ifstream fin(fileName);
  std::vector<char> buffer(1024, 0);
  unsigned long alreadyRead = 0;
  while (!fin.eof()) {
    fin.read(buffer.data(), buffer.size());
    std::streamsize dataSize = fin.gcount();
    std::string s(buffer.begin(), buffer.begin() + dataSize);
    _textBufferInsert(s, dataSize, alreadyRead);
    alreadyRead += dataSize;
  }
}