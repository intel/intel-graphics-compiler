#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(SUPPORT_SOURCES
  GenXDiagnostic.cpp
  Status.cpp
  ShaderDump.cpp
  BackendConfig.cpp
  PassManager.cpp
  PassPrinters.cpp
  )

add_library(VCSupport ${SUPPORT_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Support
  Core
  )
target_link_libraries(VCSupport
  VCHeaders

  ${LLVM_LIBS}
  )
