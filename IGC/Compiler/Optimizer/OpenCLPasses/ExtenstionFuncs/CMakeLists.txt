#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ExtenstionFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionArgAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncsAnalysis.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ExtenstionFuncs ${IGC_BUILD__SRC__ExtenstionFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__ExtenstionFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionArgAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncsAnalysis.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ExtenstionFuncs ${IGC_BUILD__HDR__ExtenstionFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ExtenstionFuncs
    "ExtensionFuncs"
    FILES
      ${IGC_BUILD__SRC__ExtenstionFuncs}
      ${IGC_BUILD__HDR__ExtenstionFuncs}
  )
