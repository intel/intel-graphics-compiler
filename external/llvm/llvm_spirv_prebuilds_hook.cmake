#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Handle translator build with prebuilt LLVM.

include_guard(DIRECTORY)

if(IGC_OPTION__LINK_KHRONOS_SPIRV_TRANSLATOR)

# If we are using prebuilds then nothing to do here.
if(NOT IGC_BUILD__SPIRV_TRANSLATOR_SOURCES)
  return()
endif()

# Translator can be added with LLVM.
if(TARGET LLVMSPIRVLib)
  message(STATUS "[SPIRV] Translator is built with LLVM")
  return()
endif()

message(STATUS "[SPIRV] Building SPIRV with prebuilt LLVM")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/spirv ${CMAKE_CURRENT_BINARY_DIR}/spirv)

endif()
