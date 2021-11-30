#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(DRIVER_SOURCES
  Driver.cpp
  SPIRVWrapper.cpp
  )

add_library(VCDriver ${DRIVER_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Analysis
  BitReader
  Core
  Support
  CodeGen
  ipo
  )

target_link_libraries(VCDriver
  LLVMGenXIntrinsics
  IGCOptions
  VCCodeGen
  VCSupport

  ${LLVM_LIBS}
  )

if(LLVM_ON_UNIX)
  target_link_libraries(VCDriver dl)
endif()

target_link_libraries(VCDriver LLVMSPIRVLib)
