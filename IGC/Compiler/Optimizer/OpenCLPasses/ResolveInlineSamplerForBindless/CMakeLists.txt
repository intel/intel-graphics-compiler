#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ResolveInlineSamplerForBindless
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveInlineSamplerForBindless.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ResolveInlineSamplerForBindless ${IGC_BUILD__SRC__ResolveInlineSamplerForBindless} PARENT_SCOPE)

set(IGC_BUILD__HDR__ResolveInlineSamplerForBindless
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveInlineSamplerForBindless.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ResolveInlineSamplerForBindless ${IGC_BUILD__HDR__ResolveInlineSamplerForBindless} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ResolveInlineSamplerForBindless
    "ResolveInlineSamplerForBindless"
    FILES
      ${IGC_BUILD__SRC__ResolveInlineSamplerForBindless}
      ${IGC_BUILD__HDR__ResolveInlineSamplerForBindless}
  )
