#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BindlessImage
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveInlineSamplerForBindless.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrepareInlineSamplerForBindless.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveImageImplicitArgsForBindless.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BindlessImage ${IGC_BUILD__SRC__BindlessImage} PARENT_SCOPE)

set(IGC_BUILD__HDR__BindlessImage
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveInlineSamplerForBindless.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrepareInlineSamplerForBindless.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveImageImplicitArgsForBindless.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BindlessImage ${IGC_BUILD__HDR__BindlessImage} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BindlessImage
    "BindlessImage"
    FILES
      ${IGC_BUILD__SRC__BindlessImage}
      ${IGC_BUILD__HDR__BindlessImage}
  )
