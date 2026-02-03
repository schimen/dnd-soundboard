cmake_minimum_required(VERSION 3.28.3)
project(dnd-soundboard)

set (CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Werror)

# Required packages
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(sdbus-c++ REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(argparse REQUIRED)
pkg_search_module(LIBEVDEV REQUIRED libevdev)

include(GNUInstallDirs)

add_subdirectory(apps)
