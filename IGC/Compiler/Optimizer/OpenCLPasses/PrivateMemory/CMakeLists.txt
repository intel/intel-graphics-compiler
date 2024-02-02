#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__PrivateMemory
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryUsageAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryToSLM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerByValAttribute.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_PrivateMemory ${IGC_BUILD__SRC__PrivateMemory} PARENT_SCOPE)

set(IGC_BUILD__HDR__PrivateMemory
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryUsageAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrivateMemoryToSLM.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LowerByValAttribute.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_PrivateMemory ${IGC_BUILD__HDR__PrivateMemory} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_PrivateMemory
    "PrivateMemory"
    FILES
      ${IGC_BUILD__SRC__PrivateMemory}
      ${IGC_BUILD__HDR__PrivateMemory}
  )
