#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Raytracing
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveOCLRaytracingBuiltins.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Raytracing ${IGC_BUILD__SRC__Raytracing} PARENT_SCOPE)

set(IGC_BUILD__HDR__Raytracing
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveOCLRaytracingBuiltins.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Raytracing ${IGC_BUILD__HDR__Raytracing} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Raytracing
    "Raytracing"
    FILES
      ${IGC_BUILD__SRC__Raytracing}
      ${IGC_BUILD__HDR__Raytracing}
  )
