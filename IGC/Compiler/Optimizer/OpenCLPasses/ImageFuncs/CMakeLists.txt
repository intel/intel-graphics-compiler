#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ImageFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ImageFuncResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ImageFuncsAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveSampledImageBuiltins.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ImageFuncs ${IGC_BUILD__SRC__ImageFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__ImageFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ImageFuncResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ImageFuncsAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveSampledImageBuiltins.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ImageFuncs ${IGC_BUILD__HDR__ImageFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ImageFuncs
    "ImageFuncs"
    FILES
      ${IGC_BUILD__SRC__ImageFuncs}
      ${IGC_BUILD__HDR__ImageFuncs}
  )