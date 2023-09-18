#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ErrorCheckPass
    "${CMAKE_CURRENT_SOURCE_DIR}/ErrorCheckPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ErrorCheckPass ${IGC_BUILD__SRC__ErrorCheckPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__ErrorCheckPass
    "${CMAKE_CURRENT_SOURCE_DIR}/ErrorCheckPass.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ErrorCheckPass ${IGC_BUILD__HDR__ErrorCheckPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ErrorCheckPass
    "ErrorCheckPass"
    FILES
      ${IGC_BUILD__SRC__ErrorCheckPass}
      ${IGC_BUILD__HDR__ErrorCheckPass}
  )