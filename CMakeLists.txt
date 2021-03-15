cmake_minimum_required(VERSION 3.17)
project(dbedit)

set(CMAKE_CXX_STANDARD 14)

add_executable(dbedit main.cpp editor_config.cpp editor_config.h splay_tree.cpp splay_tree.h utils.cpp utils.h)