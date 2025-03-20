#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Add some default paths where SPIRV can be placed.
if (WIN32)
  list(APPEND IGC_LLVM_SPIRV_PATHS
    ${CMAKE_CURRENT_LIST_DIR}/../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}
    ${CMAKE_CURRENT_LIST_DIR}/../../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}
    )
else()
  list(APPEND IGC_LLVM_SPIRV_PATHS
    ${CMAKE_CURRENT_LIST_DIR}/../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}
    ${CMAKE_CURRENT_LIST_DIR}/../../../../SPIRV-LLVM-Translator_${IGC_OPTION__LLVM_PREFERRED_VERSION}
    )
endif()
list(APPEND IGC_LLVM_SPIRV_PATHS
  ${CMAKE_CURRENT_LIST_DIR}/../../../llvm-spirv
  ${CMAKE_CURRENT_LIST_DIR}/../../../../llvm-spirv
  )

message(STATUS "IGC_LLVM_SPIRV_PATHS: ${IGC_LLVM_SPIRV_PATHS}")
find_path(DEFAULT_SPIRV_TRANSLATOR_SOURCE_DIR
  "CMakeLists.txt"
  PATHS ${IGC_LLVM_SPIRV_PATHS}
  NO_DEFAULT_PATH
  )
