#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__SpvPredicatedIOResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvPredicatedIOResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_SpvPredicatedIOResolution ${IGC_BUILD__SRC__SpvPredicatedIOResolution} PARENT_SCOPE)

set(IGC_BUILD__HDR__SpvPredicatedIOResolution
    "${CMAKE_CURRENT_SOURCE_DIR}/SpvPredicatedIOResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_SpvPredicatedIOResolution ${IGC_BUILD__HDR__SpvPredicatedIOResolution} PARENT_SCOPE)

igc_sg_register(
    Compiler__OpenCLPasses_SpvPredicatedIOResolution
    "SpvPredicatedIOResolution"
    FILES
      ${IGC_BUILD__SRC__SpvPredicatedIOResolution}
      ${IGC_BUILD__HDR__SpvPredicatedIOResolution}
  )