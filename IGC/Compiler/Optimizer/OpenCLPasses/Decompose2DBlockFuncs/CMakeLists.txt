#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Decompose2DBlockFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/Decompose2DBlockFuncs.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Decompose2DBlockFuncs ${IGC_BUILD__SRC__Decompose2DBlockFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__Decompose2DBlockFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/Decompose2DBlockFuncs.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Decompose2DBlockFuncs ${IGC_BUILD__HDR__Decompose2DBlockFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Decompose2DBlockFuncs
    "Decompose2DBlockFuncs"
    FILES
      ${IGC_BUILD__SRC__Decompose2DBlockFuncs}
      ${IGC_BUILD__HDR__Decompose2DBlockFuncs}
  )
