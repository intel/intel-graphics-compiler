#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(IGC_BUILD__SRC__7_0
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineAddSub.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineAndOrXor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCalls.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCasts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineCompares.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineLoadStoreAlloca.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineMulDivRem.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombinePHI.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineShifts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineSimplifyDemanded.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineVectorOps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstructionCombining.cpp"
  )
set(IGC_BUILD__SRC__IGCInstCombiner_7_0 ${IGC_BUILD__SRC__7_0} PARENT_SCOPE)

set(IGC_BUILD__HDR__7_0
    "${CMAKE_CURRENT_SOURCE_DIR}/InstCombineInternal.h"
  )
set(IGC_BUILD__HDR__IGCInstCombiner_7_0 ${IGC_BUILD__HDR__7_0} PARENT_SCOPE)

igc_sg_register(
    Optimizer__IGCInstCombiner_7_0
    "7.0"
    FILES
      ${IGC_BUILD__SRC__7_0}
      ${IGC_BUILD__HDR__7_0}
  )
