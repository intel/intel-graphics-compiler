#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ProcessBICodeAssumption
    "${CMAKE_CURRENT_SOURCE_DIR}/ProcessBICodeAssumption.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ProcessBICodeAssumption ${IGC_BUILD__SRC__ProcessBICodeAssumption} PARENT_SCOPE)

set(IGC_BUILD__HDR__ProcessBICodeAssumption
    "${CMAKE_CURRENT_SOURCE_DIR}/ProcessBICodeAssumption.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ProcessBICodeAssumption ${IGC_BUILD__HDR__ProcessBICodeAssumption} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ProcessBICodeAssumption
    "ProcessBICodeAssumption"
    FILES
      ${IGC_BUILD__SRC__ProcessBICodeAssumption}
      ${IGC_BUILD__HDR__ProcessBICodeAssumption}
  )
