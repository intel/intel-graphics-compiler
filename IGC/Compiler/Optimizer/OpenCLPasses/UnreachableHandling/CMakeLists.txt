#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__UnreachableHandling
    "${CMAKE_CURRENT_SOURCE_DIR}/UnreachableHandling.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_UnreachableHandling ${IGC_BUILD__SRC__UnreachableHandling} PARENT_SCOPE)

set(IGC_BUILD__HDR__UnreachableHandling
    "${CMAKE_CURRENT_SOURCE_DIR}/UnreachableHandling.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_UnreachableHandling ${IGC_BUILD__HDR__UnreachableHandling} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_UnreachableHandling
    "UnreachableHandling"
    FILES
      ${IGC_BUILD__SRC__UnreachableHandling}
      ${IGC_BUILD__HDR__UnreachableHandling}
  )