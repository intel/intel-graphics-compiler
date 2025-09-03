#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DisableInliningPass
    "${CMAKE_CURRENT_SOURCE_DIR}/DisableInlining.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DisableInliningPass ${IGC_BUILD__SRC__DisableInliningPass} PARENT_SCOPE)

set(IGC_BUILD__HDR__DisableInliningPass
    "${CMAKE_CURRENT_SOURCE_DIR}/DisableInlining.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DisableInliningPass ${IGC_BUILD__HDR__DisableInliningPass} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DisableInliningPass
    "DisableInliningPass"
    FILES
      ${IGC_BUILD__SRC__DisableInliningPass}
      ${IGC_BUILD__HDR__DisableInliningPass}
  )