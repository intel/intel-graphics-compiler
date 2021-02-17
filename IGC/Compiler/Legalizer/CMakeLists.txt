#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2010-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")


set(IGC_BUILD__SRC__Legalizer
    "${CMAKE_CURRENT_SOURCE_DIR}/InstElementizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstExpander.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstLegalChecker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstPromoter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstScalarizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstSoftener.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TypeLegalizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PeepholeTypeLegalizer.cpp"
  )
set(IGC_BUILD__SRC__Compiler_Legalizer ${IGC_BUILD__SRC__Legalizer} PARENT_SCOPE)

set(IGC_BUILD__HDR__Legalizer
    "${CMAKE_CURRENT_SOURCE_DIR}/InstElementizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstExpander.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstLegalChecker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstPromoter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstScalarizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstSoftener.h"
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