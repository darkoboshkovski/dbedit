cmake_minimum_required(VERSION 3.19)
project(dbedit)

set(CMAKE_CXX_STANDARD 14)

add_executable(dbedit main.cpp utils.cpp utils.h editor_config.cpp editor_config.h splay_tree.cpp splay_tree.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

target_link_libraries(dbedit ${GTK3_LIBRARIES})
