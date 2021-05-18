#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_library(CMCLInterface INTERFACE)

target_include_directories(CMCLInterface
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
