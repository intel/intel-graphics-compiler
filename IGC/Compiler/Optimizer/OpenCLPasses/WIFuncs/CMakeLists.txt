#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__WIFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/WIFuncResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WIFuncsAnalysis.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_WIFuncs ${IGC_BUILD__SRC__WIFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__WIFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/WIFuncResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WIFuncsAnalysis.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_WIFuncs ${IGC_BUILD__HDR__WIFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_WIFuncs
    "WIFuncs"
    FILES
      ${IGC_BUILD__SRC__WIFuncs}
      ${IGC_BUILD__HDR__WIFuncs}
  )
