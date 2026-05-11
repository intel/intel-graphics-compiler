#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")



set(IGC_BUILD__SRC__Subgroup2DBlockIoResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/Subgroup2DBlockIoResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Subgroup2DBlockIoResolution ${IGC_BUILD__SRC__Subgroup2DBlockIoResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__Subgroup2DBlockIoResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/Subgroup2DBlockIoResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Subgroup2DBlockIoResolution ${IGC_BUILD__HDR__Subgroup2DBlockIoResolution} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Subgroup2DBlockIoResolution
    "Subgroup2DBlockIoResolution"
    FILES
      ${IGC_BUILD__SRC__Subgroup2DBlockIoResolution}
      ${IGC_BUILD__HDR__Subgroup2DBlockIoResolution}
  )
