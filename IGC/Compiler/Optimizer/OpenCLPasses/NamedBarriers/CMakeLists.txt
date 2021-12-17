#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__NamedBarriers
    "${CMAKE_CURRENT_SOURCE_DIR}/NamedBarriersResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_NamedBarriers ${IGC_BUILD__SRC__NamedBarriers} PARENT_SCOPE)

set(IGC_BUILD__HDR__NamedBarriers
    "${CMAKE_CURRENT_SOURCE_DIR}/NamedBarriersResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_NamedBarriers ${IGC_BUILD__HDR__NamedBarriers} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_NamedBarriers
    "NamedBarriers"
    FILES
      ${IGC_BUILD__SRC__NamedBarriers}
      ${IGC_BUILD__HDR__NamedBarriers}
  )