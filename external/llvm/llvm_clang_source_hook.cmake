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

# Source hook for clang. IGC will try to search for clang in
# find_opencl_clang script independenty of this file. All of these is
# required to handle intel opencl-clang project patching scheme.
# FIXME: this should be moved to opencl-clang repo.

include_guard(DIRECTORY)

llvm_define_mode_variable(Clang IGC_OPTION__CLANG_MODE)

# Reuse LLVM source search module. Clang should be located in the same place.
if(NOT DEFAULT_IGC_LLVM_SOURCES_DIR)
  include(llvm_source_path)
endif()
if(DEFAULT_IGC_LLVM_SOURCES_DIR)
  set(DEFAULT_IGC_CLANG_SOURCES_DIR ${DEFAULT_IGC_LLVM_SOURCES_DIR}/clang)
endif()

if(IGC_OPTION__CLANG_MODE STREQUAL SOURCE_MODE_NAME)
  set(IGC_OPTION__CLANG_SOURCES_DIR "${DEFAULT_IGC_CLANG_SOURCES_DIR}" CACHE PATH "Path to Clang sources")
endif()

# In Prebuild mode there is nothing to do here for now.
if(IGC_OPTION__CLANG_MODE STREQUAL PREBUILDS_MODE_NAME)
  return()
endif()

# No mode was specified, start searching.
if(NOT IGC_OPTION__CLANG_MODE)
  message(STATUS "[Clang] No mode was specified, searching for Clang")
  if(EXISTS "${DEFAULT_IGC_CLANG_SOURCES_DIR}")
    set(IGC_OPTION__CLANG_SOURCES_DIR "${DEFAULT_IGC_CLANG_SOURCES_DIR}")
  else()
    return()
  endif()
endif()

set(IGC_COPIED_CLANG_DIR ${IGC_LLVM_WORKSPACE_SRC}/clang)

message(STATUS "[Clang] Clang will be built from sources")
message(STATUS "[Clang] Copying stock Clang sources ${IGC_OPTION__CLANG_SOURCES_DIR} to ${IGC_COPIED_CLANG_DIR}")

if(NOT EXISTS "${IGC_COPIED_CLANG_DIR}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__CLANG_SOURCES_DIR} ${IGC_COPIED_CLANG_DIR})
endif()

# Just register clang as external llvm project.
register_llvm_external_project(clang ${IGC_COPIED_CLANG_DIR})
