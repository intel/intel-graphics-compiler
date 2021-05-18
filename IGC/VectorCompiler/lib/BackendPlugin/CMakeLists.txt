#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(LLVM_ON_UNIX)
  include(./Linux.cmake)
  return()
endif()
