#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(CM_CL_SUPPORT_SOURCES
  BuiltinTranslator.cpp
  TranslatorPass.cpp
  InitializePasses.cpp
  )

add_library(CMCLSupport ${CM_CL_SUPPORT_SOURCES})

# LLVM dependencies
igc_get_llvm_targets(LLVM_LIBS
  Core
  Support
  )

target_link_libraries(CMCLSupport
  CMCLInterface
  LLVMGenXIntrinsics
  ${LLVM_LIBS}
  )
