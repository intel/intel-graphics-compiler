#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(GENERAL_UTILS_SOURCES
  BiF.cpp
  InstRebuilder.cpp
  Types.cpp
  FunctionAttrs.cpp
  DebugInfo.cpp
  IRBuilder.cpp
  IndexFlattener.cpp
  )

add_library(VCGeneralUtils ${GENERAL_UTILS_SOURCES})
igc_get_llvm_targets(LLVM_LIBS
  Core
  Support
  BitReader
  )

# General utils should not depend on GenX/VC related libs, e.g. vc-intrinsics.
# There's GenX utils for such things.
target_link_libraries(VCGeneralUtils
  VCHeaders
  ${LLVM_LIBS}
  )
