#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.13.4)

project ("ZEBinaryUtilities")

find_package(LLVM REQUIRED CONFIG)


message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(VS_DEBUG_COMPILER_OPTIONS
    /MTd
)

set (CMAKE_C_FLAGS "-DZEBinStandAloneBuild")
set (CMAKE_CXX_FLAGS "-DZEBinStandAloneBuild")

# Include sub-projects.
add_subdirectory ("zebin")
add_subdirectory ("tools")

