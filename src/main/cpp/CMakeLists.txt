cmake_minimum_required(VERSION 3.10)

project(hyperscan_chimera CXX)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

include(configs.cmake)
include(FindPkgConfig)
include(FindJava)
include(FindJNI)

# Find Hyperscan library
pkg_check_modules(LIBHYPERSCAN REQUIRED libhs)
include_directories(${LIBHYPERSCAN_INCLUDE_DIRS})

# Find Chimera library
pkg_check_modules(LIBCHIMERA REQUIRED libch)
include_directories(${LIBCHIMERA_INCLUDE_DIRS})

# Find PCRE library absolute path
find_library(LIBPCRE pcre REQUIRED)

add_subdirectory(jni)
