#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Decompose2DBlockFuncsWithHoisting
    "${CMAKE_CURRENT_SOURCE_DIR}/Decompose2DBlockFuncsWithHoisting.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Decompose2DBlockFuncsWithHoisting ${IGC_BUILD__SRC__Decompose2DBlockFuncsWithHoisting} PARENT_SCOPE)

set(IGC_BUILD__HDR__Decompose2DBlockFuncsWithHoisting
    "${CMAKE_CURRENT_SOURCE_DIR}/Decompose2DBlockFuncsWithHoisting.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Decompose2DBlockFuncsWithHoisting ${IGC_BUILD__HDR__Decompose2DBlockFuncsWithHoisting} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Decompose2DBlockFuncsWithHoisting
    "Decompose2DBlockFuncsWithHoisting"
    FILES
      ${IGC_BUILD__SRC__Decompose2DBlockFuncsWithHoisting}
      ${IGC_BUILD__HDR__Decompose2DBlockFuncsWithHoisting}
  )
