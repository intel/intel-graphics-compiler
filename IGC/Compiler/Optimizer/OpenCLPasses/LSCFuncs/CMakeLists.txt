#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__LSCFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/LSCFuncsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_LSCFuncs ${IGC_BUILD__SRC__LSCFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__LSCFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/LSCFuncsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_LSCFuncs ${IGC_BUILD__HDR__LSCFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_LSCFuncs
    "LSCFuncs"
    FILES
      ${IGC_BUILD__SRC__LSCFuncs}
      ${IGC_BUILD__HDR__LSCFuncs}
  )
