#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__GEPLoopStrengthReduction
    "${CMAKE_CURRENT_SOURCE_DIR}/GEPLoopStrengthReduction.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_GEPLoopStrengthReduction ${IGC_BUILD__SRC__GEPLoopStrengthReduction} PARENT_SCOPE)

set(IGC_BUILD__HDR__GEPLoopStrengthReduction
    "${CMAKE_CURRENT_SOURCE_DIR}/GEPLoopStrengthReduction.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_GEPLoopStrengthReduction ${IGC_BUILD__HDR__GEPLoopStrengthReduction} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_GEPLoopStrengthReduction
    "GEPLoopStrengthReduction"
    FILES
      ${IGC_BUILD__SRC__GEPLoopStrengthReduction}
      ${IGC_BUILD__HDR__GEPLoopStrengthReduction}
  )
