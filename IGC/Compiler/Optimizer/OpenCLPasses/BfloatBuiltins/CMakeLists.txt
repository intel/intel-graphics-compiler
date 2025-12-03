#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__BfloatBuiltins
    "${CMAKE_CURRENT_SOURCE_DIR}/BfloatBuiltinsResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_BfloatBuiltins ${IGC_BUILD__SRC__BfloatBuiltins} PARENT_SCOPE)

set(IGC_BUILD__HDR__BfloatBuiltins
    "${CMAKE_CURRENT_SOURCE_DIR}/BfloatBuiltinsResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_BfloatBuiltins ${IGC_BUILD__HDR__BfloatBuiltins} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_BfloatBuiltins
    "BfloatBuiltins"
    FILES
      ${IGC_BUILD__SRC__BfloatBuiltins}
      ${IGC_BUILD__HDR__BfloatBuiltins}
  )
