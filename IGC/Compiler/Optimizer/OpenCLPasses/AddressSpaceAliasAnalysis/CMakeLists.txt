#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__AddressSpaceAliasAnalysis
    "${CMAKE_CURRENT_SOURCE_DIR}/AddressSpaceAliasAnalysis.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_AddressSpaceAliasAnalysis ${IGC_BUILD__SRC__AddressSpaceAliasAnalysis} PARENT_SCOPE)

set(IGC_BUILD__HDR__AddressSpaceAliasAnalysis
    "${CMAKE_CURRENT_SOURCE_DIR}/AddressSpaceAliasAnalysis.h"
  )
set(IGC_BUILD__HDR__OpenCLPasses_AddressSpaceAliasAnalysis ${IGC_BUILD__HDR__AddressSpaceAliasAnalysis} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_AddressSpaceAliasAnalysis
    "AddressSpaceAliasAnalysis"
    FILES
      ${IGC_BUILD__SRC__AddressSpaceAliasAnalysis}
      ${IGC_BUILD__HDR__AddressSpaceAliasAnalysis}
  )