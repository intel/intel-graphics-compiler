#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DebuggerSupport
    "${CMAKE_CURRENT_SOURCE_DIR}/ImplicitGIDPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ImplicitGIDRestoring.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DebuggerSupport ${IGC_BUILD__SRC__DebuggerSupport} PARENT_SCOPE)

set(IGC_BUILD__HDR__DebuggerSupport
    "${CMAKE_CURRENT_SOURCE_DIR}/ImplicitGIDPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ImplicitGIDRestoring.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DebuggerSupport ${IGC_BUILD__HDR__DebuggerSupport} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DebuggerSupport
    "DebuggerSupport"
    FILES
      ${IGC_BUILD__SRC__DebuggerSupport}
      ${IGC_BUILD__HDR__DebuggerSupport}
  )
