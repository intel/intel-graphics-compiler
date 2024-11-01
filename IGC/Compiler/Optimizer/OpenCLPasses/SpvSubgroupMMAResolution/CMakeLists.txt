#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__SpvSubgroupMMAResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvSubgroupMMAResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SpvSubgroupMMAResolution ${IGC_BUILD__SRC__SpvSubgroupMMAResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__SpvSubgroupMMAResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvSubgroupMMAResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SpvSubgroupMMAResolution ${IGC_BUILD__HDR__SpvSubgroupMMAResolution} PARENT_SCOPE)

igc_sg_register(
    Compiler__OpenCLPasses_SpvSubgroupMMAResolution
    "SpvSubgroupMMAResolution"
    FILES
      ${IGC_BUILD__SRC__SpvSubgroupMMAResolution}
      ${IGC_BUILD__HDR__SpvSubgroupMMAResolution}
  )
