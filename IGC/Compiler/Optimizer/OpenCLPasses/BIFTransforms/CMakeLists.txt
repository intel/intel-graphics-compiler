#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BIFTransforms
    "${CMAKE_CURRENT_SOURCE_DIR}/BIFTransforms.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BIFTransforms ${IGC_BUILD__SRC__BIFTransforms} PARENT_SCOPE)

set(IGC_BUILD__HDR__BIFTransforms
    "${CMAKE_CURRENT_SOURCE_DIR}/BIFTransforms.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BIFTransforms ${IGC_BUILD__HDR__BIFTransforms} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BIFTransforms
    "BIFTransforms"
    FILES
      ${IGC_BUILD__SRC__BIFTransforms}
      ${IGC_BUILD__HDR__BIFTransforms}
  )
