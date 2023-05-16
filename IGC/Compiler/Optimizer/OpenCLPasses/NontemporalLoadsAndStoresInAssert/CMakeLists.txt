#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__NontemporalLoadsAndStoresInAssert
    "${CMAKE_CURRENT_SOURCE_DIR}/NontemporalLoadsAndStoresInAssert.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_NontemporalLoadsAndStoresInAssert ${IGC_BUILD__SRC__NontemporalLoadsAndStoresInAssert} PARENT_SCOPE)

set(IGC_BUILD__HDR__NontemporalLoadsAndStoresInAssert
    "${CMAKE_CURRENT_SOURCE_DIR}/NontemporalLoadsAndStoresInAssert.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_NontemporalLoadsAndStoresInAssert ${IGC_BUILD__HDR__NontemporalLoadsAndStoresInAssert} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_NontemporalLoadsAndStoresInAssert
    "NontemporalLoadsAndStoresInAssert"
    FILES
      ${IGC_BUILD__SRC__NontemporalLoadsAndStoresInAssert}
      ${IGC_BUILD__HDR__NontemporalLoadsAndStoresInAssert}
  )