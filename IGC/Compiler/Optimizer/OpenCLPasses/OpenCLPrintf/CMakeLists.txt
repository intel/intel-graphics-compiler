#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__OpenCLPrintf
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenCLPrintfAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenCLPrintfResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_OpenCLPrintf ${IGC_BUILD__SRC__OpenCLPrintf} PARENT_SCOPE)

set(IGC_BUILD__HDR__OpenCLPrintf
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenCLPrintfAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenCLPrintfResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_OpenCLPrintf ${IGC_BUILD__HDR__OpenCLPrintf} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_OpenCLPrintf
    "OpenCLPrintf"
    FILES
      ${IGC_BUILD__SRC__OpenCLPrintf}
      ${IGC_BUILD__HDR__OpenCLPrintf}
  )