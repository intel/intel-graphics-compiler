#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__FoldZeroInitAllocaIntoMemset
    "${CMAKE_CURRENT_SOURCE_DIR}/FoldZeroInitAllocaIntoMemset.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_FoldZeroInitAllocaIntoMemset ${IGC_BUILD__SRC__FoldZeroInitAllocaIntoMemset} PARENT_SCOPE)

set(IGC_BUILD__HDR__FoldZeroInitAllocaIntoMemset
    "${CMAKE_CURRENT_SOURCE_DIR}/FoldZeroInitAllocaIntoMemset.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_FoldZeroInitAllocaIntoMemset ${IGC_BUILD__HDR__FoldZeroInitAllocaIntoMemset} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_FoldZeroInitAllocaIntoMemset
    "FoldZeroInitAllocaIntoMemset"
    FILES
      ${IGC_BUILD__SRC__FoldZeroInitAllocaIntoMemset}
      ${IGC_BUILD__HDR__FoldZeroInitAllocaIntoMemset}
  )
