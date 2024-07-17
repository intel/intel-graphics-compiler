#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__MinimumValidAddressChecking
    "${CMAKE_CURRENT_SOURCE_DIR}/MinimumValidAddressChecking.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_MinimumValidAddressChecking ${IGC_BUILD__SRC__MinimumValidAddressChecking} PARENT_SCOPE)

set(IGC_BUILD__HDR__MinimumValidAddressChecking
    "${CMAKE_CURRENT_SOURCE_DIR}/MinimumValidAddressChecking.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_MinimumValidAddressChecking ${IGC_BUILD__HDR__MinimumValidAddressChecking} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_MinimumValidAddressChecking
    "MinimumValidAddressChecking"
    FILES
      ${IGC_BUILD__SRC__MinimumValidAddressChecking}
      ${IGC_BUILD__HDR__MinimumValidAddressChecking}
  )
