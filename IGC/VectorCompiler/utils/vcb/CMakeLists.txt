#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(VCB_EXECUTABLE_NAME "vcb")

add_executable(${VCB_EXECUTABLE_NAME}
  vcb.cpp
  UniqueCompilation.cpp
  )

igc_get_llvm_targets(LLVM_LIBS
  Core
  Support
  )

target_link_libraries("${VCB_EXECUTABLE_NAME}"
  VCCodeGen
  VCDriver
  VCHeaders
  ${LLVM_LIBS}
  )

