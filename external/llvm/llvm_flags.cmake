#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

include(llvm_utils.cmake)

# Do not build any backends.
set_llvm_opt(LLVM_TARGETS_TO_BUILD "" CACHE STRING "desc")

# Required to run LIT tests.
set_llvm_opt(LLVM_INCLUDE_TOOLS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_BUILD_TOOLS OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_INCLUDE_UTILS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_BUILD_UTILS OFF CACHE BOOL "desc")

set_llvm_opt(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_INCLUDE_TESTS OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_APPEND_VC_REV OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_THREADS ON CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_PIC ON CACHE BOOL "desc")
set_llvm_opt(LLVM_ABI_BREAKING_CHECKS FORCE_OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_DUMP ON CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_TERMINFO OFF CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_EH ON CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_RTTI ON CACHE BOOL "desc")

set_llvm_opt(LLVM_ENABLE_EH ON CACHE BOOL "desc")
set_llvm_opt(LLVM_ENABLE_RTTI ON CACHE BOOL "desc")
if ("${ARCH}" STREQUAL "32")
  set_llvm_opt(LLVM_BUILD_32_BITS ON CACHE BOOL "desc")
else()
  set_llvm_opt(LLVM_BUILD_32_BITS OFF CACHE BOOL "desc")
endif()
