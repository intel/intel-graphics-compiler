#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__ScalarDebugInfo
    "${CMAKE_CURRENT_SOURCE_DIR}/VISAScalarModule.cpp")
set(IGC_BUILD__SRC__Compiler_ScalarDebugInfo ${IGC_BUILD__SRC__ScalarDebugInfo} PARENT_SCOPE)

set(IGC_BUILD__HDR__ScalarDebugInfo
    "${CMAKE_CURRENT_SOURCE_DIR}/VISAScalarModule.hpp")
set(IGC_BUILD__HDR__Compiler_ScalarDebugInfo ${IGC_BUILD__HDR__ScalarDebugInfo} PARENT_SCOPE)

igc_sg_register(
    Compiler__ScalarDebugInfo
    "ScalarDebugInfo"
    FILES
      ${IGC_BUILD__SRC__ScalarDebugInfo}
      ${IGC_BUILD__HDR__ScalarDebugInfo}
  )
