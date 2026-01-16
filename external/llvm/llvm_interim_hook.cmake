#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Handles setting up LLVM interim build.
# Changes LLVM build mode to Source and handles dependencies.

if(NOT IGC_BUILD_LLVM_INTERIM)
  return()
endif()

message(STATUS "[LLVM] Configuring LLVM for Interim build")

if(NOT IGC_OPTION__LLVM_PREFERRED_VERSION)
  message(FATAL_ERROR "IGC_OPTION__LLVM_PREFERRED_VERSION should be defined!")
endif()

set(IGC_OPTION__LLVM_MODE "Source" CACHE STRING "" FORCE)
set(IGC_OPTION__LLVM_STOCK_SOURCES OFF CACHE BOOL "" FORCE)
set(IGC_OPTION__LLVM_SOURCES_DIR "${DEFAULT_IGC_LLVM_SOURCES_DIR}" CACHE PATH "Path to LLVM sources" FORCE)

if(IGC_OPTION__LLVM_SOURCES_DIR STREQUAL "")
  message(FATAL_ERROR "IGC_OPTION__LLVM_SOURCES_DIR could not be determined for Interim build!")
endif()

set(IGC_OPTION__LLD_MODE "Source" CACHE STRING "" FORCE)
set(IGC_OPTION__lld_SOURCES_DIR ${IGC_OPTION__LLVM_SOURCES_DIR}/lld CACHE PATH "Path to lld sources when building LLVM" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_MODULE_PATH})

include(llvm_flags)
set_llvm_opt(LLVM_BUILD_LLVM_DYLIB OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_LINK_LLVM_DYLIB OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_BUILD_UTILS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_INSTALL_UTILS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_BUILD_TOOLS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_INCLUDE_BENCHMARKS OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_INCLUDE_TESTS OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_PROJECTS "" CACHE STRING "desc")

include(llvm_lld_source_hook)

set_property(DIRECTORY ${LLVM_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL OFF)
