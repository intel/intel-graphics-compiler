#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(LLVM_LINK_COMPONENTS
  Core
  Support
  CodeGen
  GenXCodeGen
  GenXOpts
  )

add_genx_unittest(RegionsTests
  OverlapTest.cpp
  )


target_include_directories(RegionsTests PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/GenXCodeGen")
target_link_libraries(RegionsTests PRIVATE LLVMTestingSupport)


