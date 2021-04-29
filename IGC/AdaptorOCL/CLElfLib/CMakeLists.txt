#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# We require cmake 3.2.0 or later
cmake_minimum_required(VERSION 3.2.0 FATAL_ERROR)

# We want to organize our IDE targets into folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# This is the main CLElfLib project's cmakelists.txt file
project(CLElfLib)

# Set up a comprehensive source list
set(CLELFLIB_READER_SRCS
    ${CLELFLIB_READER_SRCS}
    "${CMAKE_CURRENT_SOURCE_DIR}/ElfReader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElfReader.h"
)

set(CLELFLIB_WRITER_SRCS
    ${CLELFLIB_WRITER_SRCS}
    "${CMAKE_CURRENT_SOURCE_DIR}/ElfWriter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ElfWriter.h"
)

set(CLELFLIB_SRCS
    ${CLELFLIB_SRCS}
    "${CMAKE_CURRENT_SOURCE_DIR}/CLElfTypes.h"
    ${CLELFLIB_READER_SRCS}
    ${CLELFLIB_WRITER_SRCS}
)

# We want to create a lib so that everything in it can be tested
add_library(CLElfLib STATIC ${CLELFLIB_SRCS})

set_target_properties(CLElfLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The following filters are added to the VS IDE representation
source_group("source files" FILES "${CMAKE_CURRENT_SOURCE_DIR}/CLElfTypes.h")
source_group("source files\\Reader" FILES ${CLELFLIB_READER_SRCS})
source_group("source files\\Writer" FILES ${CLELFLIB_WRITER_SRCS})

# group the igdrcl specific projects
set_target_properties(CLElfLib PROPERTIES FOLDER "elf utilities")


target_link_libraries(CLElfLib ${ASAN_LIB} ${TSAN_LIB})
set_property(TARGET CLElfLib APPEND_STRING PROPERTY COMPILE_FLAGS ${ASAN_FLAGS} ${TSAN_FLAGS})
