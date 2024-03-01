#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BfloatFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/BfloatFuncsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BfloatFuncs ${IGC_BUILD__SRC__BfloatFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__BfloatFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/BfloatFuncsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BfloatFuncs ${IGC_BUILD__HDR__BfloatFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BfloatFuncs
    "BfloatFuncs"
    FILES
      ${IGC_BUILD__SRC__BfloatFuncs}
      ${IGC_BUILD__HDR__BfloatFuncs}
  )
