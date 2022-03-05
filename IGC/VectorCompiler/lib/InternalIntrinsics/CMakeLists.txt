#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_COMPONENTS
  CodeGen
  Support
  Core
  Analysis
)

igc_get_llvm_targets(LLVM_LIBS ${LLVM_COMPONENTS})
add_library(VCInternalIntrinsics
  InternalIntrinsics.cpp
)

add_dependencies(VCInternalIntrinsics VCInternalIntrinsicsGen)
target_link_libraries(VCInternalIntrinsics
  ${LLVM_LIBS}
  VCHeaders
  LLVMGenXIntrinsics
)
