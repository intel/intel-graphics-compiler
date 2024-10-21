#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Spv2dBlockIOResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/Spv2dBlockIOResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Spv2dBlockIOResolution ${IGC_BUILD__SRC__Spv2dBlockIOResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__Spv2dBlockIOResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/Spv2dBlockIOResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Spv2dBlockIOResolution ${IGC_BUILD__HDR__Spv2dBlockIOResolution} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Spv2dBlockIOResolution
    "Spv2dBlockIOResolution"
    FILES
      ${IGC_BUILD__SRC__Spv2dBlockIOResolution}
      ${IGC_BUILD__HDR__Spv2dBlockIOResolution}
  )
