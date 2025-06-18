#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__MergeScalarPhisPass
    "${CMAKE_CURRENT_SOURCE_DIR}/MergeScalarPhisPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_MergeScalarPhisPass ${IGC_BUILD__SRC__MergeScalarPhisPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__MergeScalarPhisPass
    "${CMAKE_CURRENT_SOURCE_DIR}/MergeScalarPhisPass.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_MergeScalarPhisPass ${IGC_BUILD__HDR__MergeScalarPhisPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_MergeScalarPhisPass
    "MergeScalarPhisPass"
    FILES
      ${IGC_BUILD__SRC__MergeScalarPhisPass}
      ${IGC_BUILD__HDR__MergeScalarPhisPass}
  )
