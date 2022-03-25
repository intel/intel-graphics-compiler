#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
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
  message(STATUS "[lld] Will use lld prebuilds")
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

if(NOT EXISTS "${IGC_LLVM_WORKSPACE_SRC}/libunwind/include/mach-o" AND ${IGC_OPTION__LLVM_PREFERRED_VERSION} GREATER_EQUAL "12.0.0")
    # Need to copy one header from unwind package for LLD (only for building from sources)
    file(MAKE_DIRECTORY ${IGC_LLVM_WORKSPACE_SRC}/libunwind/include/mach-o)
    file(COPY ${DEFAULT_IGC_LLVM_SOURCES_DIR}/libunwind/include/mach-o/compact_unwind_encoding.h
         DESTINATION ${IGC_LLVM_WORKSPACE_SRC}/libunwind/include/mach-o/)
endif()

# Just register lld as external llvm project.
register_llvm_external_project(lld ${IGC_COPIED_LLD_DIR})