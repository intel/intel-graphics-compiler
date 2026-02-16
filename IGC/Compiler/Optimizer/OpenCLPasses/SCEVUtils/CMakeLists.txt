#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__SCEVUtils
    "${CMAKE_CURRENT_SOURCE_DIR}/SCEVUtils.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SCEVUtils ${IGC_BUILD__SRC__SCEVUtils} PARENT_SCOPE)

set(IGC_BUILD__HDR__SCEVUtils
    "${CMAKE_CURRENT_SOURCE_DIR}/SCEVUtils.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SCEVUtils ${IGC_BUILD__HDR__SCEVUtils} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_SCEVUtils
    "SCEVUtils"
    FILES
      ${IGC_BUILD__SRC__SCEVUtils}
      ${IGC_BUILD__HDR__SCEVUtils}
  )
