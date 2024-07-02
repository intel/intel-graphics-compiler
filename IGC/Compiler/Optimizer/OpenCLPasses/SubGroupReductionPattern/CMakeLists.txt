#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__SubGroupReductionPattern
    "${CMAKE_CURRENT_SOURCE_DIR}/SubGroupReductionPattern.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SubGroupReductionPattern ${IGC_BUILD__SRC__SubGroupReductionPattern} PARENT_SCOPE)

set(IGC_BUILD__HDR__SubGroupReductionPattern
    "${CMAKE_CURRENT_SOURCE_DIR}/SubGroupReductionPattern.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SubGroupReductionPattern ${IGC_BUILD__HDR__SubGroupReductionPattern} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_SubGroupReductionPattern
    "SubGroupReductionPattern"
    FILES
      ${IGC_BUILD__SRC__SubGroupReductionPattern}
      ${IGC_BUILD__HDR__SubGroupReductionPattern}
  )
