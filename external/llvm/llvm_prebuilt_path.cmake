#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_BUILD_TYPE ${CMAKE_BUILD_TYPE})

if(DEFINED BUILD_TYPE)
  if(${BUILD_TYPE} STREQUAL "release")
    set(LLVM_BUILD_TYPE "Release")
  else()
    set(LLVM_BUILD_TYPE "Debug")
  endif()
endif()

list(APPEND DEFAULT_IGC_LLVM_PREBUILDS_DIRS "${BS_DIR_EXTERNAL_COMPONENTS}/llvm_prebuilt_windows")
list(APPEND DEFAULT_IGC_LLVM_PREBUILDS_DIRS "${BS_DIR_EXTERNAL_COMPONENTS}/prebuild-llvm")
list(APPEND DEFAULT_IGC_LLVM_PREBUILDS_DIRS "/opt/intel-llvm-static-${IGC_OPTION__LLVM_PREFERRED_VERSION}")
