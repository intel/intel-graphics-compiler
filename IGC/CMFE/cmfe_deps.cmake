#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Module to provide useful paths to CM FE library.
# Defines CMFE_INCLUDE_DIRS and INSTALL_CMFE_NAME that
# are used to provide interface library.

set(CMFE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/AdaptorCM")

if (WIN32 OR CYGWIN)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(INSTALL_CMFE_NAME "clangFEWrapper.dll")
  else()
    set(INSTALL_CMFE_NAME "clangFEWrapper32.dll")
  endif()
else()
  set(INSTALL_CMFE_NAME "libclangFEWrapper.so")
endif()
