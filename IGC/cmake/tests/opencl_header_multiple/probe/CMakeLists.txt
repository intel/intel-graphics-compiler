#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Inner project of the opencl_header_multiple reproducer. It is configured once
# per scenario by the driver in the parent directory, which reads the values
# reported below and the exit status. Everything it needs is passed in with -D.
#
# This has to be a project() rather than a `cmake -P` script because
# igc_find_opencl_clang.cmake calls add_library()/set_property(TARGET ...),
# which script mode does not provide.

cmake_minimum_required(VERSION 3.13)
project(igc_opencl_header_probe NONE)

get_filename_component(IGC_SRC "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${IGC_SRC}/IGC/cmake" "${IGC_SRC}/IGC/BiFModule/cmake")

# Enough of the surrounding build for igc_find_opencl_clang.cmake to reach the
# prebuilds branch, which is the branch the bug was reported on.
set(IGC_BUILD__GFX_DEV_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/no-such-dir")
set(LLVM_SOURCE_DIR            "${CMAKE_CURRENT_BINARY_DIR}/no-such-dir")
set(LLVM_VERSION_MAJOR 17)
set(LLVM_VERSION_MINOR 0)
set(LLVM_VERSION_PATCH 6)
set(IGC_BUILD__CLANG_VERSION "17.0.6")
set(IGC_BUILD__CLANG_VERSION_MAJOR "17")
set(CCLANG_BUILD_PREBUILDS TRUE)
set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_TEST_PREBUILDS_DIR}")

include(igc_find_opencl_clang)

list(LENGTH opencl-header _headerCount)
message(STATUS "IGC_TEST_HEADER_COUNT=${_headerCount}")
message(STATUS "IGC_TEST_HEADER_VALUE=${opencl-header}")

# The two consumers that break, replayed exactly as they appear in the tree at
# the time the bug was reported, i.e. with an unquoted dereference. That is what
# makes them a test of the value rather than of the quoting: a single path works
# unquoted, a list does not.
#
# IGC/BiFModule/CMakeLists.txt - hard error, this is the traceback in the report.
get_filename_component(opencl-headers-dir ${opencl-header} DIRECTORY)
message(STATUS "IGC_TEST_HEADER_DIR=${opencl-headers-dir}")

# IGC/BiFModule/cmake/BiFModuleCache.cmake - silent argument shift: with a list
# here get_bif_src_list() binds its arguments to the wrong values and the BiF
# checksum target ends up with no dependencies at all.
include(BiFMCGetListFiles)
set(BiFModule_SRC_LIST "")
get_bif_src_list(${opencl-header} "${IGC_SRC}/IGC/BiFModule/" BiFModule_SRC_LIST)
list(LENGTH BiFModule_SRC_LIST _srcCount)
message(STATUS "IGC_TEST_BIF_SRC_COUNT=${_srcCount}")
