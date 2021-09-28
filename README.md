# dbedit
- A tiny fully-native text editor using a Splay Tree / Piece Table architecture and a GTK GUI.
- Writen in C++
- Development in progress! 😄

## Installation

### Step 1: Install the following dependencies:
- [gcc/g++](https://linuxconfig.org/how-to-install-g-the-c-compiler-on-ubuntu-20-04-lts-focal-fossa-linux) or clang
- [CMake](https://askubuntu.com/questions/355565/how-do-i-install-the-latest-version-of-cmake-from-the-command-line)
- [gtk3](https://askubuntu.com/questions/101306/how-do-i-install-gtk-3-0)

Make sure cmake is included in your PATH.

### Step 2: Build the project
Inside the project directory, run the following command:

```cmake --build ./build --target dbedit -j 8 --```

This will create a build folder inside the current one, which will contain the dbedit executable.

### Step 3: Run the executable
Run the executable with or without an input file. e.g.

```./dbedit```

```./dbedit test-file.txt```
