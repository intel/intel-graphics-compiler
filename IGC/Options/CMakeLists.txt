#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_library(IGCOptions STATIC "")

add_subdirectory(src)
add_subdirectory(include/igc/Options)

igc_get_llvm_targets(LLVM_LIBS Option)

target_link_libraries(IGCOptions PUBLIC
  ${LLVM_LIBS}
  )

target_include_directories(IGCOptions PUBLIC
  include
  ${CMAKE_CURRENT_BINARY_DIR}/include
  )

add_dependencies(IGCOptions
  IGCOptionsTablegen
  )
