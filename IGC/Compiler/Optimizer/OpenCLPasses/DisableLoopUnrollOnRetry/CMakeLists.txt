#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DisableLoopUnrollOnRetry
    "${CMAKE_CURRENT_SOURCE_DIR}/DisableLoopUnrollOnRetry.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DisableLoopUnrollOnRetry ${IGC_BUILD__SRC__DisableLoopUnrollOnRetry} PARENT_SCOPE)

set(IGC_BUILD__HDR__DisableLoopUnrollOnRetry
    "${CMAKE_CURRENT_SOURCE_DIR}/DisableLoopUnrollOnRetry.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DisableLoopUnrollOnRetry ${IGC_BUILD__HDR__DisableLoopUnrollOnRetry} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DisableLoopUnrollOnRetry
    "DisableLoopUnrollOnRetry"
    FILES
      ${IGC_BUILD__SRC__DisableLoopUnrollOnRetry}
      ${IGC_BUILD__HDR__DisableLoopUnrollOnRetry}
  )

