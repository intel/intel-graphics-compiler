#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../../../VectorCompiler/include")

set(IGC_BUILD__SRC__DiamondChainMergePass
  "${CMAKE_CURRENT_SOURCE_DIR}/DiamondChainMergePass.cpp"
)
set(IGC_BUILD__SRC__OpenCLPasses_DiamondChainMergePass ${IGC_BUILD__SRC__DiamondChainMergePass} PARENT_SCOPE)

set(IGC_BUILD__HDR__DiamondChainMergePass
  "${CMAKE_CURRENT_SOURCE_DIR}/DiamondChainMergePass.hpp"
)
set(IGC_BUILD__HDR__OpenCLPasses_DiamondChainMergePass ${IGC_BUILD__HDR__DiamondChainMergePass} PARENT_SCOPE)

igc_sg_register(
  Compiler__OpenCLPasses_DiamondChainMergePass
  "DiamondChainMergePass"
  FILES
    ${IGC_BUILD__SRC__DiamondChainMergePass}
    ${IGC_BUILD__HDR__DiamondChainMergePass}
)
