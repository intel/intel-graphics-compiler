#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Atomics
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveOCLAtomics.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_Atomics ${IGC_BUILD__SRC__Atomics} PARENT_SCOPE)

set(IGC_BUILD__HDR__Atomics
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLAtomicsDef.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ResolveOCLAtomics.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_Atomics ${IGC_BUILD__HDR__Atomics} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_Atomics
    "Atomics"
    FILES
      ${IGC_BUILD__SRC__Atomics}
      ${IGC_BUILD__HDR__Atomics}
  )
