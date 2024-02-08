#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BufferBoundsChecking
    "${CMAKE_CURRENT_SOURCE_DIR}/BufferBoundsChecking.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BufferBoundsCheckingPatcher.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BufferBoundsChecking ${IGC_BUILD__SRC__BufferBoundsChecking} PARENT_SCOPE)

set(IGC_BUILD__HDR__BufferBoundsChecking
    "${CMAKE_CURRENT_SOURCE_DIR}/BufferBoundsChecking.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BufferBoundsCheckingPatcher.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BufferBoundsChecking ${IGC_BUILD__HDR__BufferBoundsChecking} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BufferBoundsChecking
    "BufferBoundsChecking"
    FILES
      ${IGC_BUILD__SRC__BufferBoundsChecking}
      ${IGC_BUILD__HDR__BufferBoundsChecking}
  )
