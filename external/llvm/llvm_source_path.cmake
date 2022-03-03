#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Use LLVM sources stored at fixed location
message(STATUS "[LLVM] Current value of CMAKE_CURRENT_SOURCE_DIR : ${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND IGC_LLVM_PATHS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm-project
  /opt/src/llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../llvm-project_${IGC_OPTION__LLVM_PREFERRED_VERSION}
  )

find_path(DEFAULT_IGC_LLVM_SOURCES_DIR
  README.md
  PATHS ${IGC_LLVM_PATHS}
  NO_DEFAULT_PATH
  )
