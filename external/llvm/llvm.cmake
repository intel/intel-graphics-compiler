#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

# LLVM package could be handled in two general ways: source and prebuilds.
# To specify what mode to use `IGC_OPTION__LLVM_MODE` is used.
# Possible values are: Source, Prebuilds or nothing (default one).
# If nothing is specified then LLVM package will be searched starting from source
# mode and then using prebuilds if sources are not found.
# Additional options for modes are the following.
# 1. Source:
#   IGC_OPTION__LLVM_STOCK_SOURCES - use LLVM stock sources or patched stock sources (by default OFF)
#   IGC_OPTION__LLVM_SOURCES_DIR - set path to llvm sources folder
# 2. Prebuilds:
#   LLVM_ROOT -- additional paths to search for LLVM (separated by ';') -- these are searched before system paths
#
# LLVM version can be specified using the following variable:
# IGC_OPTION__LLVM_PREFERRED_VERSION (default is 10.0.0)
#
# From cmake writing perspective there will be defined the following
# variables that can be used by IGC:
# IGC_BUILD__LLVM_SOURCES -- ON if Source path is used
# IGC_BUILD__LLVM_PREBUILDS -- ON if Prebuilds path is used.

cmake_policy(VERSION 3.13.4)

include_guard(DIRECTORY)

llvm_define_mode_variable(LLVM IGC_OPTION__LLVM_MODE)

# Get default source dir.
include(llvm_source_path)
# Get default prebuild dirs.
include(llvm_prebuilt_path)

# Handle dependent options for Source mode.
if(IGC_OPTION__LLVM_MODE STREQUAL SOURCE_MODE_NAME)
  option(IGC_OPTION__LLVM_STOCK_SOURCES "Use stock or patched sources" OFF)
  set(IGC_OPTION__LLVM_SOURCES_DIR "${DEFAULT_IGC_LLVM_SOURCES_DIR}" CACHE PATH "Path to LLVM sources")
  # Tell the build that we are using sources.
  set(IGC_BUILD__LLVM_SOURCES ON)
endif()

# Handle dependent options for Prebuild mode.
if(IGC_OPTION__LLVM_MODE STREQUAL PREBUILDS_MODE_NAME)
  set(LLVM_ROOT "${DEFAULT_IGC_LLVM_PREBUILDS_DIRS}" CACHE PATH
    "Paths to LLVM prebuild (multiple paths can be specified separated by ;")
  # Return early as only source build is processed here.
  return()
endif()

# No mode was specified, start searching.
if(NOT IGC_OPTION__LLVM_MODE)
  message(STATUS "[LLVM] No LLVM mode was selected explicitly")
  message(STATUS "[LLVM] Search will be performed for LLVM sources first, then for prebuilds (including system LLVM)")

  # Check by order first available way to link with LLVM.
  if(EXISTS "${DEFAULT_IGC_LLVM_SOURCES_DIR}")
    set(IGC_BUILD__LLVM_SOURCES ON)
    set(IGC_OPTION__LLVM_SOURCES_DIR ${DEFAULT_IGC_LLVM_SOURCES_DIR})
  else()
    # Set defaults and stop. Prebuilds will be processed later by IGC.
    set(LLVM_ROOT ${DEFAULT_IGC_LLVM_PREBUILDS_DIRS})
    return()
  endif()
endif()

# This definitely should exist. Either user specified mode and
# directory explicitly, or we found directory during search process.
if(NOT EXISTS "${IGC_OPTION__LLVM_SOURCES_DIR}")
  message(FATAL_ERROR "[LLVM] Cannot find LLVM sources, please provide sources path by IGC_OPTION__LLVM_SOURCES_DIR flag")
endif()

message(STATUS "[LLVM] LLVM will be built from sources")
message(STATUS "[LLVM] LLVM sources folder: ${IGC_OPTION__LLVM_SOURCES_DIR}")
message(STATUS "[LLVM] LLVM sources in stock version: ${IGC_OPTION__LLVM_STOCK_SOURCES}")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR} ${IGC_LLVM_WORKSPACE}/build)

# Some variables are lost after LLVM subdirectory left.
# Restore them for IGC usage.
set(LLVM_CMAKE_DIR "${LLVM_SOURCE_DIR}/cmake/modules")
# Tools binary dir is needed by lit testing.
get_directory_property(LLVM_TOOLS_BINARY_DIR DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_TOOLS_BINARY_DIR")
