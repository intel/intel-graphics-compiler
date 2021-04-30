#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__RewriteLocalSize
    "${CMAKE_CURRENT_SOURCE_DIR}/RewriteLocalSize.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_RewriteLocalSize ${IGC_BUILD__SRC__RewriteLocalSize} PARENT_SCOPE)

set(IGC_BUILD__HDR__RewriteLocalSize
    "${CMAKE_CURRENT_SOURCE_DIR}/RewriteLocalSize.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_RewriteLocalSize ${IGC_BUILD__HDR__RewriteLocalSize} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_RewriteLocalSize
    "RewriteLocalSize"
    FILES
      ${IGC_BUILD__SRC__RewriteLocalSize}
      ${IGC_BUILD__HDR__RewriteLocalSize}
  )
