#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(cmfe_deps)

message(STATUS
  "[CMFE] CM Frontend headers are taken from: ${CMFE_INCLUDE_DIRS}")
message(STATUS
  "[CMFE] Using CMFE library name: ${INSTALL_CMFE_NAME}")

add_subdirectory(AdaptorCM)
