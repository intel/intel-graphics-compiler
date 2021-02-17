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

if(LLVM_VERSION_MAJOR EQUAL 4)
  add_subdirectory(4.0)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner ${IGC_BUILD__SRC__IGCInstCombiner_4_0} PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner ${IGC_BUILD__HDR__IGCInstCombiner_4_0} PARENT_SCOPE)
elseif(LLVM_VERSION_MAJOR EQUAL 7)
  add_subdirectory(7.0)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner ${IGC_BUILD__SRC__IGCInstCombiner_7_0} PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner ${IGC_BUILD__HDR__IGCInstCombiner_7_0} PARENT_SCOPE)
elseif(LLVM_VERSION_MAJOR VERSION_GREATER 7)
  set(IGC_BUILD__SRC__Optimizer_IGCInstCombiner "" PARENT_SCOPE)
  set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner "" PARENT_SCOPE)
  message("Use InstCombiner from LLVM")
else()
  message(FATAL_ERROR "Error: unsupported version of LLVM: ${LLVM_VERSION_MAJOR}.")
endif()

set(IGC_BUILD__HDR__Optimizer_IGCInstCombiner
    "${IGC_BUILD__HDR__Optimizer_IGCInstCombiner}"
    "${CMAKE_CURRENT_SOURCE_DIR}/IGCInstructionCombining.h"
    )

if(LLVM_VERSION_MAJOR EQUAL 4 OR
   LLVM_VERSION_MAJOR EQUAL 7)
igc_sg_register(
    Compiler__Optimizer_IGCInstCombiner
    "IGCInstCombiner"
    GROUPS
      Optimizer__IGCInstCombiner_4_0
      Optimizer__IGCInstCombiner_7_0
    FILES
  )
endif()