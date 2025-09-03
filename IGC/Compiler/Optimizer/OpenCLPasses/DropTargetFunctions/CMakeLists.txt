#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DropTargetFunctionsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/DropTargetFunctions.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DropTargetFunctionsPass ${IGC_BUILD__SRC__DropTargetFunctionsPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__DropTargetFunctionsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/DropTargetFunctions.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DropTargetFunctionsPass ${IGC_BUILD__HDR__DropTargetFunctionsPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DropTargetFunctionsPass
    "DropTargetFunctions"
    FILES
      ${IGC_BUILD__SRC__DropTargetFunctionsPass}
      ${IGC_BUILD__HDR__DropTargetFunctionsPass}
  )