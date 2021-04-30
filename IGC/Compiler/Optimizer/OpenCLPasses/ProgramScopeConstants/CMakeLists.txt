#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__ProgramScopeConstants
    "${CMAKE_CURRENT_SOURCE_DIR}/ProgramScopeConstantAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ProgramScopeConstantResolution.cpp"
  )
set(IGC_BUILD__SRC__OpenCLPasses_ProgramScopeConstants ${IGC_BUILD__SRC__ProgramScopeConstants} PARENT_SCOPE)

set(IGC_BUILD__HDR__ProgramScopeConstants
    "${CMAKE_CURRENT_SOURCE_DIR}/ProgramScopeConstantAnalysis.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ProgramScopeConstantResolution.hpp"
  )
set(IGC_BUILD__HDR__OpenCLPasses_ProgramScopeConstants ${IGC_BUILD__HDR__ProgramScopeConstants} PARENT_SCOPE)


igc_sg_register(
    Compiler__OpenCLPasses_ProgramScopeConstants
    "ProgramScopeConstant"
    FILES
      ${IGC_BUILD__SRC__ProgramScopeConstants}
      ${IGC_BUILD__HDR__ProgramScopeConstants}
  )