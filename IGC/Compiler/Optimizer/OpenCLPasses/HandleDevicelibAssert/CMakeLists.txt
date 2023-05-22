#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__HandleDevicelibAssert
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleDevicelibAssert.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_HandleDevicelibAssert ${IGC_BUILD__SRC__HandleDevicelibAssert} PARENT_SCOPE)

set(IGC_BUILD__HDR__HandleDevicelibAssert
    "${CMAKE_CURRENT_SOURCE_DIR}/HandleDevicelibAssert.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_HandleDevicelibAssert ${IGC_BUILD__HDR__HandleDevicelibAssert} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_HandleDevicelibAssert
    "HandleDevicelibAssert"
    FILES
      ${IGC_BUILD__SRC__HandleDevicelibAssert}
      ${IGC_BUILD__HDR__HandleDevicelibAssert}
  )
  