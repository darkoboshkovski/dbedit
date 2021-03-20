// *** Includes ***
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k)&0x1f)

#include "editor_config.h"
#include "splay_tree.h"
#include "utils.h"

// TODO investigate working splaying into remove
// TODO look into undo/redo
// TODO holding key disrupts insert
// TODO add tab
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
  logFile.close();
  E.n_words.clear();
}
void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;

  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
  //  raw.c_oflag &= ~(OPOST);
  raw.c_lflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}
char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

int getWindowSize(unsigned long *rows, unsigned long *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

// *** Output ***
void editorDrawRows() {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "~\n", 2);
  }
}

void editorClearScreen() { write(STDOUT_FILENO, "\x1b[2J", 4); }
void editorRefreshScreen() {
  // Clear whole screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // Move cursor to (1,1)
  write(STDOUT_FILENO, "\x1b[H", 3);

  //    editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

// *** Input ***
void arrowKeyPressHandler(int key) {
  // 65 - up, 66 - down, 67 - right, 68 - left
  int new_row, new_col;
  switch (key) {
  case 65:
    new_row = std::max(E.row - 1, (unsigned long)1);
    new_col = std::min(E.screencols, E.n_words[new_row] + 1);
    break;
  case 66:
    new_row = std::min(std::min(E.row + 1, E.screenrows), E.n_rows);
    new_col = std::min(E.screencols, E.n_words[new_row] + 1);
    break;
  case 68:
    new_row = E.row;
    new_col = std::max(E.column - 1, (unsigned long)1);
    break;
  case 67:
    new_row = E.row;
    new_col =
        std::min(std::min(E.column + 1, E.n_words[new_row] + 1), E.screencols);
    break;
  default:
    break;
  }
  editorMoveCursorToPosition(new_row, new_col);
}
unsigned long calculatePosition(unsigned long row, unsigned long column) {
  // number of newlines
  unsigned long position = row - 1;
  for (int i = 1; i < row; i++)
    position += E.n_words[i];
  position += column - 1;
  return position;
}
void handleReturn() {

  unsigned long position = calculatePosition(E.row, E.column);
  char newLine = '\n';
  textBufferInsert(&newLine, 1, position);
  editorMoveCursorToPosition(E.row + 1, 1);
}

void handleBackspace() {
  int position = calculatePosition(E.row, E.column) - 1;
  if (position < 0)
    return;
  textBufferRemove(position, position);
  unsigned long new_row, new_column;
  if (E.column == 1) {
    new_row = std::max(E.row - 1, (unsigned long)1);
    new_column = E.n_words[new_row] + 1;
  } else {
    new_row = E.row;
    new_column = E.column - 1;
  }
  editorMoveCursorToPosition(new_row, new_column);
}

bool editorProcessKeypress() {
  char c = editorReadKey();
  bool rerenderAfterProcess = false;
  if (c == CTRL_KEY('q')) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
  } else if (c == 13) {
    handleReturn();
    rerenderAfterProcess = true;
  } else if (c >= 65 && c <= 68 && E.control_char == 2) {
    arrowKeyPressHandler((int)c);
  } else if (c == 127 || c == 8) {
    handleBackspace();
    rerenderAfterProcess = true;
  } else if (c == '\t') {
    int position = calculatePosition(E.row, E.column);
    textBufferInsert("    ", 4, position);
    E.column += 4;
    rerenderAfterProcess = true;
  } else if (iscntrl(c) == 0 && E.control_char == 0) {
    /* write(STDOUT_FILENO, &c, 1); */
    /* *(E.n_words + E.row) += 1; */
    /* E.column++; */
    int position = calculatePosition(E.row, E.column);
    textBufferInsert(std::string(1, c), 1, position);
    E.column++;
    rerenderAfterProcess = true;
  }

  // Control character hack. TODO
  if (c == '\x1b' && E.control_char == 0) {
    E.control_char++;
  } else if (c == '[' && E.control_char == 1) {
    E.control_char++;
  } else
    E.control_char = 0;

  return rerenderAfterProcess;
}

void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
  E.row = 1;
  E.column = 1;
  std::vector<unsigned long> v(1024, 0);
  E.n_words = v;
  E.control_char = 0;
  // gapBufferInit();
}

int main(int argc, char **argv) {
  logFile.open("../log.txt", std::ofstream::out);
  enableRawMode();
  //  std::cout<<"d\nb\n"<<std::endl;
  //  return 0;
  initEditor();
  logFile << "d\nb\n";
  logFile << "SEE HERE " << (int)'\n' << std::endl;
  if (argc > 1) {
    // There is a file to open
    std::string readFileName = argv[1];
    readFile(readFileName);
  }
  editorRefreshScreen();
  displayText(0, UINT32_MAX);
  logFile << "SCREEN " << E.screenrows << " " << E.screencols << std::endl;
  while (1) {
    /* gapBufferDisplay(); */
    if (editorProcessKeypress()) {
      editorRefreshScreen();
      displayText(0, UINT32_MAX);
    }
  }
}
