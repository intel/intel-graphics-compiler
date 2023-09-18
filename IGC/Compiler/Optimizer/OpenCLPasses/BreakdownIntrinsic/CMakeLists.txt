#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BreakdownIntrinsic
    "${CMAKE_CURRENT_SOURCE_DIR}/BreakdownIntrinsic.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BreakdownIntrinsic ${IGC_BUILD__SRC__BreakdownIntrinsic} PARENT_SCOPE)

set(IGC_BUILD__HDR__BreakdownIntrinsic
    "${CMAKE_CURRENT_SOURCE_DIR}/BreakdownIntrinsic.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BreakdownIntrinsic ${IGC_BUILD__HDR__BreakdownIntrinsic} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BreakdownIntrinsic
    "BreakdownIntrinsic"
    FILES
      ${IGC_BUILD__SRC__BreakdownIntrinsic}
      ${IGC_BUILD__HDR__BreakdownIntrinsic}
  )