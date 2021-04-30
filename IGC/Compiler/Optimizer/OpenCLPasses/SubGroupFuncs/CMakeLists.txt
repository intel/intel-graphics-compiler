#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__SubGroupFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/SubGroupFuncsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SubGroupFuncs ${IGC_BUILD__SRC__SubGroupFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__SubGroupFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/SubGroupFuncsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SubGroupFuncs ${IGC_BUILD__HDR__SubGroupFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_SubGroupFuncs
    "SubGroupFuncs"
    FILES
      ${IGC_BUILD__SRC__SubGroupFuncs}
      ${IGC_BUILD__HDR__SubGroupFuncs}
  )