#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_LINK_COMPONENTS
  Core
  Support
  CodeGen
  GenXIntrinsics
  SPIRVLib
  )

add_genx_unittest(SPIRVConversionsTests
  SPIRVConversionsTest.cpp
  )

target_include_directories(SPIRVConversionsTests PRIVATE ${SPIRV_INCLUDE_DIR})
target_link_libraries(SPIRVConversionsTests PRIVATE LLVMTestingSupport)


