#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ClampLoopUnroll
    "${CMAKE_CURRENT_SOURCE_DIR}/ClampLoopUnroll.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ClampLoopUnroll ${IGC_BUILD__SRC__ClampLoopUnroll} PARENT_SCOPE)

set(IGC_BUILD__HDR__ClampLoopUnroll
    "${CMAKE_CURRENT_SOURCE_DIR}/ClampLoopUnroll.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ClampLoopUnroll ${IGC_BUILD__HDR__ClampLoopUnroll} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ClampLoopUnroll
    "ClampLoopUnroll"
    FILES
      ${IGC_BUILD__SRC__ClampLoopUnroll}
      ${IGC_BUILD__HDR__ClampLoopUnroll}
  )
