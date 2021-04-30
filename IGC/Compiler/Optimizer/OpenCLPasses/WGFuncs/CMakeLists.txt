#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__WGFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/WGFuncResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_WGFuncs ${IGC_BUILD__SRC__WGFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__WGFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/WGFuncResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_WGFuncs ${IGC_BUILD__HDR__WGFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_WGFuncs
    "WGFuncs"
    FILES
      ${IGC_BUILD__SRC__WGFuncs}
      ${IGC_BUILD__HDR__WGFuncs}
  )