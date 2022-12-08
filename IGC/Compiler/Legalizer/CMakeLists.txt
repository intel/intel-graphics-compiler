#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Legalizer
    "${CMAKE_CURRENT_SOURCE_DIR}/InstLegalChecker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstPromoter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TypeLegalizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PeepholeTypeLegalizer.cpp"
  )
set(IGC_BUILD__SRC__Compiler_Legalizer ${IGC_BUILD__SRC__Legalizer} PARENT_SCOPE)

set(IGC_BUILD__HDR__Legalizer
    "${CMAKE_CURRENT_SOURCE_DIR}/InstLegalChecker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstPromoter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/TypeLegalizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/TypeLegalizerPass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PeepholeTypeLegalizer.hpp"
  )
set(IGC_BUILD__HDR__Compiler_Legalizer ${IGC_BUILD__HDR__Legalizer} PARENT_SCOPE)


igc_sg_register(
    Compiler__Legalizer
    "Legalizer"
    FILES
      ${IGC_BUILD__SRC__Legalizer}
      ${IGC_BUILD__HDR__Legalizer}
  )
