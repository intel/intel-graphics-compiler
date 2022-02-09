#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# adding target VCInternalIntrinsicsGen - generation of internal intrinsic information
add_subdirectory(vc/InternalIntrinsics)

# Special common target for headers that propagates
# needed include directories and dependencies.
add_library(VCHeaders INTERFACE)

target_include_directories(VCHeaders
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_dependencies(VCHeaders
  intrinsics_gen
  VCInternalIntrinsicsGen
  )
