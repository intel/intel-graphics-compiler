#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__DeviceEnqueueFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/DeviceEnqueue.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_DeviceEnqueueFuncs ${IGC_BUILD__SRC__DeviceEnqueueFuncs} PARENT_SCOPE)

set(IGC_BUILD__HDR__DeviceEnqueueFuncs
    "${CMAKE_CURRENT_SOURCE_DIR}/DeviceEnqueue.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_DeviceEnqueueFuncs ${IGC_BUILD__HDR__DeviceEnqueueFuncs} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_DeviceEnqueueFuncs
    "DeviceEnqueue"
    FILES
      ${IGC_BUILD__SRC__DeviceEnqueueFuncs}
      ${IGC_BUILD__HDR__DeviceEnqueueFuncs}
  )
