#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__PoisonFP64KernelsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/PoisonFP64KernelsPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_PoisonFP64KernelsPass ${IGC_BUILD__SRC__PoisonFP64KernelsPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__PoisonFP64KernelsPass
    "${CMAKE_CURRENT_SOURCE_DIR}/PoisonFP64KernelsPass.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_PoisonFP64KernelsPass ${IGC_BUILD__HDR__PoisonFP64KernelsPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_PoisonFP64KernelsPass
    "PoisonFP64KernelsPass"
    FILES
      ${IGC_BUILD__SRC__PoisonFP64KernelsPass}
      ${IGC_BUILD__HDR__PoisonFP64KernelsPass}
  )