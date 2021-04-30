#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ReplaceUnsupportedIntrinsics
    "${CMAKE_CURRENT_SOURCE_DIR}/ReplaceUnsupportedIntrinsics.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ReplaceUnsupportedIntrinsics ${IGC_BUILD__SRC__ReplaceUnsupportedIntrinsics} PARENT_SCOPE)

set(IGC_BUILD__HDR__ReplaceUnsupportedIntrinsics
    "${CMAKE_CURRENT_SOURCE_DIR}/ReplaceUnsupportedIntrinsics.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ReplaceUnsupportedIntrinsics ${IGC_BUILD__HDR__ReplaceUnsupportedIntrinsics} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ReplaceUnsupportedIntrinsics
    "ReplaceUnsupportedIntrinsics"
    FILES
      ${IGC_BUILD__SRC__ReplaceUnsupportedIntrinsics}
      ${IGC_BUILD__HDR__ReplaceUnsupportedIntrinsics}
  )