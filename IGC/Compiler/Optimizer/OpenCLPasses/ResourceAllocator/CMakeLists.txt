#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ResourceAllocator
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceAllocator.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ResourceAllocator ${IGC_BUILD__SRC__ResourceAllocator} PARENT_SCOPE)

set(IGC_BUILD__HDR__ResourceAllocator
    "${CMAKE_CURRENT_SOURCE_DIR}/ResourceAllocator.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ResourceAllocator ${IGC_BUILD__HDR__ResourceAllocator} PARENT_SCOPE)