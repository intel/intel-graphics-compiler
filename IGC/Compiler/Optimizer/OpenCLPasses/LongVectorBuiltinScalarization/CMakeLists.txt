#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__LongVectorBuiltinScalarization
    "${CMAKE_CURRENT_SOURCE_DIR}/LongVectorBuiltinScalarization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LongVectorMangling.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_LongVectorBuiltinScalarization ${IGC_BUILD__SRC__LongVectorBuiltinScalarization} PARENT_SCOPE)

set(IGC_BUILD__HDR__LongVectorBuiltinScalarization
    "${CMAKE_CURRENT_SOURCE_DIR}/LongVectorBuiltinScalarization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LongVectorMangling.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_LongVectorBuiltinScalarization ${IGC_BUILD__HDR__LongVectorBuiltinScalarization} PARENT_SCOPE)

igc_sg_register(
    Compiler__OpenCLPasses_LongVectorBuiltinScalarization
    "LongVectorBuiltinScalarization"
    FILES
      ${IGC_BUILD__SRC__LongVectorBuiltinScalarization}
      ${IGC_BUILD__HDR__LongVectorBuiltinScalarization}
  )
