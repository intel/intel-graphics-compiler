#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_library(CMCLHeaders INTERFACE)

target_include_directories(CMCLHeaders
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/Headers
  )

add_subdirectory(Support)
