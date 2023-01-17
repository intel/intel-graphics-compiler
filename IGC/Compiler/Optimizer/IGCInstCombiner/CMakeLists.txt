#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner "" PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner "" PARENT_SCOPE)

set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner
    "${IGC_BUILD__HDR__Optimizer_IGCInstCombiner}"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCInstructionCombining.h"
    )
