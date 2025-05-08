#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__SplitStructurePhisPass
    "${CMAKE_CURRENT_SOURCE_DIR}/SplitStructurePhisPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SplitStructurePhisPass ${IGC_BUILD__SRC__SplitStructurePhisPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__SplitStructurePhisPass
    "${CMAKE_CURRENT_SOURCE_DIR}/SplitStructurePhisPass.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SplitStructurePhisPass ${IGC_BUILD__HDR__SplitStructurePhisPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_SplitStructurePhisPass
    "SplitStructurePhisPass"
    FILES
      ${IGC_BUILD__SRC__SplitStructurePhisPass}
      ${IGC_BUILD__HDR__SplitStructurePhisPass}
  )
