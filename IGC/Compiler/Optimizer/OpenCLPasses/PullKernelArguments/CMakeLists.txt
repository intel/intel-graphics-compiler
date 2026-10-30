#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__PullKernelArguments
    "${CMAKE_CURRENT_SOURCE_DIR}/PullKernelArguments.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_PullKernelArguments ${IGC_BUILD__SRC__PullKernelArguments} PARENT_SCOPE)

set(IGC_BUILD__HDR__PullKernelArguments
    "${CMAKE_CURRENT_SOURCE_DIR}/PullKernelArguments.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_PullKernelArguments ${IGC_BUILD__HDR__PullKernelArguments} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_PullKernelArguments
    "PullKernelArguments"
    FILES
      ${IGC_BUILD__SRC__PullKernelArguments}
      ${IGC_BUILD__HDR__PullKernelArguments}
  )
  