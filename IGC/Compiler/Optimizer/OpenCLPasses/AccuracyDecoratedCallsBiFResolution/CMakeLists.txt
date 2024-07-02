#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__AccuracyDecoratedCallsBiFResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/AccuracyDecoratedCallsBiFResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_AccuracyDecoratedCallsBiFResolution ${IGC_BUILD__SRC__AccuracyDecoratedCallsBiFResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__AccuracyDecoratedCallsBiFResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/NameToBuiltinDef.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AccuracyDecoratedCallsBiFResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_AccuracyDecoratedCallsBiFResolution ${IGC_BUILD__HDR__AccuracyDecoratedCallsBiFResolution} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_AccuracyDecoratedCallsBiFResolution
    "AccuracyDecoratedCallsBiFResolution"
    FILES
      ${IGC_BUILD__SRC__AccuracyDecoratedCallsBiFResolution}
      ${IGC_BUILD__HDR__AccuracyDecoratedCallsBiFResolution}
  )
