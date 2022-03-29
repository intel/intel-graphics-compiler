#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Add some default paths where SPIRV can be placed.
list(APPEND IGC_LLVM_SPIRV_PATHS
  ${CMAKE_CURRENT_LIST_DIR}/../../../SPIRV-LLVM-Translator
  ${CMAKE_CURRENT_LIST_DIR}/../../../../SPIRV-LLVM-Translator
  ${CMAKE_CURRENT_LIST_DIR}/../../../llvm-spirv
  ${CMAKE_CURRENT_LIST_DIR}/../../../../llvm-spirv
  )

message(STATUS "IGC_LLVM_SPIRV_PATHS: ${IGC_LLVM_SPIRV_PATHS}")
find_path(DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR
  "CMakeLists.txt"
  PATHS ${IGC_LLVM_SPIRV_PATHS}
  NO_DEFAULT_PATH
  )
