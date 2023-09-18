#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__TransformUnmaskedFunctionsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/TransformUnmaskedFunctionsPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_TransformUnmaskedFunctionsPass ${IGC_BUILD__SRC__TransformUnmaskedFunctionsPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__TransformUnmaskedFunctionsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/TransformUnmaskedFunctionsPass.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_TransformUnmaskedFunctionsPass ${IGC_BUILD__HDR__TransformUnmaskedFunctionsPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_TransformUnmaskedFunctionsPass
    "TransformUnmaskedFunctionsPass"
    FILES
      ${IGC_BUILD__SRC__TransformUnmaskedFunctionsPass}
      ${IGC_BUILD__HDR__TransformUnmaskedFunctionsPass}
  )