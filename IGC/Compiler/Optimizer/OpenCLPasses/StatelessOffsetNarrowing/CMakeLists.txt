#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__StatelessOffsetNarrowing
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessOffsetNarrowing.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_StatelessOffsetNarrowing ${IGC_BUILD__SRC__StatelessOffsetNarrowing} PARENT_SCOPE)

set(IGC_BUILD__HDR__StatelessOffsetNarrowing
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessOffsetNarrowing.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_StatelessOffsetNarrowing ${IGC_BUILD__HDR__StatelessOffsetNarrowing} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_StatelessOffsetNarrowing
    "StatelessOffsetNarrowing"
    FILES
      ${IGC_BUILD__SRC__StatelessOffsetNarrowing}
      ${IGC_BUILD__HDR__StatelessOffsetNarrowing}
  )