#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__KernelArgs
    "${CMAKE_CURRENT_SOURCE_DIR}/KernelArgs.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_KernelArgs ${IGC_BUILD__SRC__KernelArgs} PARENT_SCOPE)

set(IGC_BUILD__HDR__KernelArgs
    "${CMAKE_CURRENT_SOURCE_DIR}/KernelArgs.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_KernelArgs ${IGC_BUILD__HDR__KernelArgs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_KernelArgs
    "KernelArgs"
    FILES
      ${IGC_BUILD__SRC__KernelArgs}
      ${IGC_BUILD__HDR__KernelArgs}
  )