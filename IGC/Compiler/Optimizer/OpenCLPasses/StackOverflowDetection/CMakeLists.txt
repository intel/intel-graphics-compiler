#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__StackOverflowDetection
    "${CMAKE_CURRENT_SOURCE_DIR}/StackOverflowDetection.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_StackOverflowDetection ${IGC_BUILD__SRC__StackOverflowDetection} PARENT_SCOPE)

set(IGC_BUILD__HDR__StackOverflowDetection
    "${CMAKE_CURRENT_SOURCE_DIR}/StackOverflowDetection.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_StackOverflowDetection ${IGC_BUILD__HDR__StackOverflowDetection} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_StackOverflowDetection
    "StackOverflowDetection"
    FILES
      ${IGC_BUILD__SRC__StackOverflowDetection}
      ${IGC_BUILD__HDR__StackOverflowDetection}
  )
