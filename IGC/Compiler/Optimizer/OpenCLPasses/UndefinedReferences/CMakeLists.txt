#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__UndefinedReferences
    "${CMAKE_CURRENT_SOURCE_DIR}/UndefinedReferencesPass.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_UndefinedReferences ${IGC_BUILD__SRC__UndefinedReferences} PARENT_SCOPE)

set(IGC_BUILD__HDR__UndefinedReferences
    "${CMAKE_CURRENT_SOURCE_DIR}/UndefinedReferencesPass.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_UndefinedReferences ${IGC_BUILD__HDR__UndefinedReferences} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_UndefinedReferences
    "UndefinedReferences"
    FILES
      ${IGC_BUILD__SRC__UndefinedReferences}
      ${IGC_BUILD__HDR__UndefinedReferences}
  )