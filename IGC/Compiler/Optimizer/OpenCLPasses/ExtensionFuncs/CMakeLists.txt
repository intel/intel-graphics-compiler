#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ExtensionFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionArgAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncResolution.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncsAnalysis.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ExtensionFuncs ${IGC_BUILD__SRC__ExtensionFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__ExtensionFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionArgAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncResolution.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExtensionFuncsAnalysis.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ExtensionFuncs ${IGC_BUILD__HDR__ExtensionFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ExtensionFuncs
    "ExtensionFuncs"
    FILES
      ${IGC_BUILD__SRC__ExtensionFuncs}
      ${IGC_BUILD__HDR__ExtensionFuncs}
  )
