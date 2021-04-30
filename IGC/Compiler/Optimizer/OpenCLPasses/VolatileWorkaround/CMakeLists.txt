#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__VolatileWorkaround
    "${CMAKE_CURRENT_SOURCE_DIR}/VolatileWorkaround.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_VolatileWorkaround ${IGC_BUILD__SRC__VolatileWorkaround} PARENT_SCOPE)

set(IGC_BUILD__HDR__VolatileWorkaround
    "${CMAKE_CURRENT_SOURCE_DIR}/VolatileWorkaround.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_VolatileWorkaround ${IGC_BUILD__HDR__VolatileWorkaround} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_VolatileWorkaround
    "VolatileWorkaround"
    FILES
      ${IGC_BUILD__SRC__VolatileWorkaround}
      ${IGC_BUILD__HDR__VolatileWorkaround}
  )