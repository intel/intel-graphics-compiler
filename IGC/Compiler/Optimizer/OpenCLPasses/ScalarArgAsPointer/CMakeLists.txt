#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ScalarArgAsPointer
    "${CMAKE_CURRENT_SOURCE_DIR}/ScalarArgAsPointer.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ScalarArgAsPointer ${IGC_BUILD__SRC__ScalarArgAsPointer} PARENT_SCOPE)

set(IGC_BUILD__HDR__ScalarArgAsPointer
    "${CMAKE_CURRENT_SOURCE_DIR}/ScalarArgAsPointer.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ScalarArgAsPointer ${IGC_BUILD__HDR__ScalarArgAsPointer} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ScalarArgAsPointer
    "ScalarArgAsPointer"
    FILES
      ${IGC_BUILD__SRC__ScalarArgAsPointer}
      ${IGC_BUILD__HDR__ScalarArgAsPointer}
  )
