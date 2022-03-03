#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(GENX_UTILS_SOURCES
  Printf.cpp
  TypeSize.cpp
  BreakConst.cpp
  RegCategory.cpp
  Region.cpp
  ImplicitArgsBuffer.cpp
  KernelInfo.cpp
  Intrinsics.cpp
  IntrinsicsWrapper.cpp
  IRBuilder.cpp
  GlobalVariable.cpp
  )

add_library(VCGenXUtils ${GENX_UTILS_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Core
  Support
  )

target_link_libraries(VCGenXUtils
  VCHeaders
  LLVMGenXIntrinsics
  VCInternalIntrinsics
  VCGeneralUtils
  ${LLVM_LIBS}
  )
