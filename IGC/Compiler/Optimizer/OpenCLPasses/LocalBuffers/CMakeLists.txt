#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__LocalBuffers
    "${CMAKE_CURRENT_SOURCE_DIR}/InlineLocalsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_LocalBuffers ${IGC_BUILD__SRC__LocalBuffers} PARENT_SCOPE)

set(IGC_BUILD__HDR__LocalBuffers
    "${CMAKE_CURRENT_SOURCE_DIR}/InlineLocalsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_LocalBuffers ${IGC_BUILD__HDR__LocalBuffers} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_LocalBuffers
    "LocalBuffers"
    FILES
      ${IGC_BUILD__SRC__LocalBuffers}
      ${IGC_BUILD__HDR__LocalBuffers}
  )