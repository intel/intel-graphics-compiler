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

# Just register lld as external llvm project.
include_guard(DIRECTORY)

llvm_define_mode_variable(LLD IGC_OPTION__LLD_MODE)

# Reuse LLVM source search module. lld should be located in the same place.
if(NOT DEFAULT_IGC_LLVM_SOURCES_DIR)
  include(llvm_source_path)
endif()
if(DEFAULT_IGC_LLVM_SOURCES_DIR)
  set(DEFAULT_IGC_lld_SOURCES_DIR ${DEFAULT_IGC_LLVM_SOURCES_DIR}/lld)
endif()

if(IGC_OPTION__LLD_MODE STREQUAL SOURCE_MODE_NAME)
  set(IGC_OPTION__lld_SOURCES_DIR "${DEFAULT_IGC_lld_SOURCES_DIR}" CACHE PATH "Path to lld sources")
endif()

# In Prebuild mode there is nothing to do here for now.
if(IGC_OPTION__LLD_MODE STREQUAL PREBUILDS_MODE_NAME)
  return()
endif()

# No mode was specified, start searching.
if(NOT IGC_OPTION__LLD_MODE)
  message(STATUS "[lld] No mode was specified, searching for lld")
  if(EXISTS "${DEFAULT_IGC_lld_SOURCES_DIR}")
    set(IGC_OPTION__lld_SOURCES_DIR "${DEFAULT_IGC_lld_SOURCES_DIR}")
  else()
    return()
  endif()
endif()

set(IGC_COPIED_LLD_DIR ${IGC_LLVM_WORKSPACE_SRC}/lld)

message(STATUS "[lld] lld will be built from sources")
message(STATUS "[lld] Copying stock lld sources ${IGC_OPTION__lld_SOURCES_DIR} to ${IGC_COPIED_LLD_DIR}")

if(NOT EXISTS "${IGC_COPIED_LLD_DIR}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${IGC_OPTION__lld_SOURCES_DIR} ${IGC_COPIED_LLD_DIR})
endif()

# Just register lld as external llvm project.
register_llvm_external_project(lld ${IGC_COPIED_LLD_DIR})