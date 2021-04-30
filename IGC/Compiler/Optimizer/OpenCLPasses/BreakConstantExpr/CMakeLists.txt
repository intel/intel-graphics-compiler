#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BreakConstantExpr
    "${CMAKE_CURRENT_SOURCE_DIR}/BreakConstantExpr.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BreakConstantExpr ${IGC_BUILD__SRC__BreakConstantExpr} PARENT_SCOPE)

set(IGC_BUILD__HDR__BreakConstantExpr
    "${CMAKE_CURRENT_SOURCE_DIR}/BreakConstantExpr.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BreakConstantExpr ${IGC_BUILD__HDR__BreakConstantExpr} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BreakConstantExpr
    "BreakConstantExpr"
    FILES
      ${IGC_BUILD__SRC__BreakConstantExpr}
      ${IGC_BUILD__HDR__BreakConstantExpr}
  )