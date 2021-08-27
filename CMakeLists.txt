#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(IGC)

add_subdirectory(IGC)

list(APPEND IGC__IGC_TARGETS "igc_dll")
list(APPEND IGC__IGC_TARGETS "IGA_DLL")
list(APPEND IGC__IGC_TARGETS "IGA_EXE")
list(APPEND IGC__IGC_TARGETS "fcl_dll")
list(APPEND IGC__IGC_TARGETS "opencl-clang-lib")

set(IGC__IGC_TARGETS "${IGC__IGC_TARGETS}" PARENT_SCOPE)
set(IGC_LIBRARY_NAME "${IGC_LIBRARY_NAME}" PARENT_SCOPE)
set(FCL_LIBRARY_NAME "${FCL_LIBRARY_NAME}" PARENT_SCOPE)
