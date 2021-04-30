#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__AlignmentAnalysis
    "${CMAKE_CURRENT_SOURCE_DIR}/AlignmentAnalysis.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_AlignmentAnalysis ${IGC_BUILD__SRC__AlignmentAnalysis} PARENT_SCOPE)

set(IGC_BUILD__HDR__AlignmentAnalysis
    "${CMAKE_CURRENT_SOURCE_DIR}/AlignmentAnalysis.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_AlignmentAnalysis ${IGC_BUILD__HDR__AlignmentAnalysis} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_AlignmentAnalysis
    "AlignmentAnalysis"
    FILES
      ${IGC_BUILD__SRC__AlignmentAnalysis}
      ${IGC_BUILD__HDR__AlignmentAnalysis}
  )