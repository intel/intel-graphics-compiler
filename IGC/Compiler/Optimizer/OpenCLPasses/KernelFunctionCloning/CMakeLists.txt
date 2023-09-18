#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__KernelFunctionCloning
    "${CMAKE_CURRENT_SOURCE_DIR}/KernelFunctionCloning.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_KernelFunctionCloning ${IGC_BUILD__SRC__KernelFunctionCloning} PARENT_SCOPE)

set(IGC_BUILD__HDR__KernelFunctionCloning
    "${CMAKE_CURRENT_SOURCE_DIR}/KernelFunctionCloning.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_KernelFunctionCloning ${IGC_BUILD__HDR__KernelFunctionCloning} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_KernelFunctionCloning
    "KernelFunctionCloning"
    FILES
      ${IGC_BUILD__SRC__KernelFunctionCloning}
      ${IGC_BUILD__HDR__KernelFunctionCloning}
  )