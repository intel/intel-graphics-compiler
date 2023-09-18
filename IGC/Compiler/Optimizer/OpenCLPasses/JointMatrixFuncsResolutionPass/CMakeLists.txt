#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__JointMatrixFuncsResolutionPass
    "${CMAKE_CURRENT_SOURCE_DIR}/JointMatrixFuncsResolutionPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_JointMatrixFuncsResolutionPass ${IGC_BUILD__SRC__JointMatrixFuncsResolutionPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__JointMatrixFuncsResolutionPass
    "${CMAKE_CURRENT_SOURCE_DIR}/JointMatrixFuncsResolutionPass.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_JointMatrixFuncsResolutionPass ${IGC_BUILD__HDR__JointMatrixFuncsResolutionPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_JointMatrixFuncsResolutionPass
    "JointMatrixFuncsResolutionPass"
    FILES
      ${IGC_BUILD__SRC__JointMatrixFuncsResolutionPass}
      ${IGC_BUILD__HDR__JointMatrixFuncsResolutionPass}
  )