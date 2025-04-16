#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__PrepareInlineSamplerForBindless
    "${CMAKE_CURRENT_SOURCE_DIR}/PrepareInlineSamplerForBindless.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_PrepareInlineSamplerForBindless ${IGC_BUILD__SRC__PrepareInlineSamplerForBindless} PARENT_SCOPE)

set(IGC_BUILD__HDR__PrepareInlineSamplerForBindless
    "${CMAKE_CURRENT_SOURCE_DIR}/PrepareInlineSamplerForBindless.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_PrepareInlineSamplerForBindless ${IGC_BUILD__HDR__PrepareInlineSamplerForBindless} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_PrepareInlineSamplerForBindless
    "PrepareInlineSamplerForBindless"
    FILES
      ${IGC_BUILD__SRC__PrepareInlineSamplerForBindless}
      ${IGC_BUILD__HDR__PrepareInlineSamplerForBindless}
  )
