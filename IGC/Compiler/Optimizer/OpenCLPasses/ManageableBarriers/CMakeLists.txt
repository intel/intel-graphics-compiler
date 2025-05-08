#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ManageableBarriers
    "${CMAKE_CURRENT_SOURCE_DIR}/ManageableBarriersResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ManageableBarriers ${IGC_BUILD__SRC__ManageableBarriers} PARENT_SCOPE)

set(IGC_BUILD__HDR__ManageableBarriers
    "${CMAKE_CURRENT_SOURCE_DIR}/ManageableBarriersResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ManageableBarriers ${IGC_BUILD__HDR__ManageableBarriers} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ManageableBarriers
    "ManageableBarriers"
    FILES
      ${IGC_BUILD__SRC__ManageableBarriers}
      ${IGC_BUILD__HDR__ManageableBarriers}
  )
