#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2017-2021 Intel Corporation
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

#
# LLVM module
#

cmake_minimum_required(VERSION 3.13.4)

find_package(Python3 REQUIRED)
find_package(Patch REQUIRED)

include(llvm_utils.cmake)
include(llvm_flags.cmake)

message(STATUS "[LLVM] : LLVM_SOURCE_URL   = ${IGC_OPTION__LLVM_SOURCES_DIR}")
message(STATUS "[LLVM] : LLVM_OPTIONS      = ${LLVM_OPTIONS}")

if(NOT IGC_OPTION__LLVM_STOCK_SOURCES)
  include(llvm_patch_sources.cmake)
else()
  set(IGC_LLVM_SOURCE_DIR ${IGC_OPTION__LLVM_SOURCES_DIR}/llvm)
endif()

include(llvm_spirv.cmake)

if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generator.
    set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")
    message(STATUS "[LLVM_PATCHER] : LLVM_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
else()
    # Single-configuration generator.
    message(STATUS "[LLVM_PATCHER] : LLVM_BUILD_TYPE   = ${CMAKE_BUILD_TYPE}")
endif()

# Do not alter LLVM compilation flags -- clear them all.
llvm_utils_push_build_flags()
llvm_utils_python_set()
# project() command sets global <project>_SOURCE_DIR and <project>_BINARY_DIR and we can use it later.
add_subdirectory(${IGC_LLVM_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src EXCLUDE_FROM_ALL)
llvm_utils_python_restore()
# No need to restore build flags since they are local to this subdirectory.

# Sanity check for version matching.
get_directory_property(LLVM_VERSION  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION")
if(NOT ("${LLVM_VERSION}" VERSION_EQUAL "${IGC_OPTION__LLVM_PREFERRED_VERSION}"))
  message(FATAL_ERROR "Found LLVM version: ${LLVM_VERSION};"
    " however preferred was: ${IGC_OPTION__LLVM_PREFERRED_VERSION}\n"
    "Please set IGC_OPTION__PREFERRED_LLVM_VERSION to ${LLVM_VERSION}"
    " or pass IGC_OPTION__LLVM_SOURCES_DIR with preferred version")
endif()


# Get version of LLVM that we are currently using.
get_directory_property(LLVM_VERSION_MAJOR  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_MAJOR")
get_directory_property(LLVM_VERSION_MINOR  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_MINOR")
get_directory_property(LLVM_VERSION_PATCH  DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_PATCH")
get_directory_property(LLVM_VERSION_SUFFIX DIRECTORY ${LLVM_SOURCE_DIR} DEFINITION "LLVM_VERSION_SUFFIX")
# Make LLVM version accessible in parent scope.
set(LLVM_VERSION_MAJOR  ${LLVM_VERSION_MAJOR}  PARENT_SCOPE)
set(LLVM_VERSION_MINOR  ${LLVM_VERSION_MINOR}  PARENT_SCOPE)
set(LLVM_VERSION_PATCH  ${LLVM_VERSION_PATCH}  PARENT_SCOPE)
set(LLVM_VERSION_SUFFIX ${LLVM_VERSION_SUFFIX} PARENT_SCOPE)

set(LLVM_INCLUDE_DIRS "${LLVM_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}/src/include")
set(LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS} PARENT_SCOPE)
