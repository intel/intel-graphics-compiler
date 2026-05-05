#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__SpvSubgroupBitcastShuffleResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvSubgroupBitcastShuffleResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SpvSubgroupBitcastShuffleResolution ${IGC_BUILD__SRC__SpvSubgroupBitcastShuffleResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__SpvSubgroupBitcastShuffleResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvSubgroupBitcastShuffleResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SpvSubgroupBitcastShuffleResolution ${IGC_BUILD__HDR__SpvSubgroupBitcastShuffleResolution} PARENT_SCOPE)

igc_sg_register(
    Compiler__OpenCLPasses_SpvSubgroupBitcastShuffleResolution
    "SpvSubgroupBitcastShuffleResolution"
    FILES
      ${IGC_BUILD__SRC__SpvSubgroupBitcastShuffleResolution}
      ${IGC_BUILD__HDR__SpvSubgroupBitcastShuffleResolution}
  )
