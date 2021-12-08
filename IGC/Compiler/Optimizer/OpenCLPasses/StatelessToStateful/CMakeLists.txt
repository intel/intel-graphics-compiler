#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__StatelessToStateful
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessToStateful.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_StatelessToStateful ${IGC_BUILD__SRC__StatelessToStateful} PARENT_SCOPE)

set(IGC_BUILD__HDR__StatelessToStateful
    "${CMAKE_CURRENT_SOURCE_DIR}/StatelessToStateful.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_StatelessToStateful ${IGC_BUILD__HDR__StatelessToStateful} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_StatelessToStateful
    "StatelessToStateful"
    FILES
      ${IGC_BUILD__SRC__StatelessToStateful}
      ${IGC_BUILD__HDR__StatelessToStateful}
  )