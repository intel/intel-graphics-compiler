#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(DEFINED IGC_OPTION__LIBRARY_NAME)
  set(IGC_LIBRARY_NAME "${IGC_OPTION__LIBRARY_NAME}")
else()
  set(IGC_LIBRARY_NAME "igc")
endif()
