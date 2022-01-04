#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DpasFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/DpasFuncsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DpasFuncs ${IGC_BUILD__SRC__DpasFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__DpasFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/DpasFuncsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DpasFuncs ${IGC_BUILD__HDR__DpasFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DpasFuncs
    "DpasFuncs"
    FILES
      ${IGC_BUILD__SRC__DpasFuncs}
      ${IGC_BUILD__HDR__DpasFuncs}
  )
