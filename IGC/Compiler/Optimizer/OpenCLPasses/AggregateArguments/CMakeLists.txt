#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__AggregateArguments
    "${CMAKE_CURRENT_SOURCE_DIR}/AggregateArguments.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_AggregateArguments ${IGC_BUILD__SRC__AggregateArguments} PARENT_SCOPE)

set(IGC_BUILD__HDR__AggregateArguments
    "${CMAKE_CURRENT_SOURCE_DIR}/AggregateArguments.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_AggregateArguments ${IGC_BUILD__HDR__AggregateArguments} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_AggregateArguments
    "AggregateArguments"
    FILES
      ${IGC_BUILD__SRC__AggregateArguments}
      ${IGC_BUILD__HDR__AggregateArguments}
  )