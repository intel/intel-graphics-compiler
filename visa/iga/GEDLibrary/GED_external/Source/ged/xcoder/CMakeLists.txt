#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# GED_external/Source/ged/xcoder
set(GED_xcoder_cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_interpreters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_restrictions_handler.cpp
  PARENT_SCOPE
)
# GED_external/Source/ged/xcoder
set(GED_xcoder_h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_internal_api.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_interpreters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_restrictions_handler.h
  PARENT_SCOPE
)
