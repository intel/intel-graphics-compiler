#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__RegPressureLoopControl
    "${CMAKE_CURRENT_SOURCE_DIR}/RegPressureLoopControl.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_RegPressureLoopControl ${IGC_BUILD__SRC__RegPressureLoopControl} PARENT_SCOPE)

set(IGC_BUILD__HDR__RegPressureLoopControl
    "${CMAKE_CURRENT_SOURCE_DIR}/RegPressureLoopControl.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_RegPressureLoopControl ${IGC_BUILD__HDR__RegPressureLoopControl} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_RegPressureLoopControl
    "RegPressureLoopControl"
    FILES
      ${IGC_BUILD__SRC__RegPressureLoopControl}
      ${IGC_BUILD__HDR__RegPressureLoopControl}
  )