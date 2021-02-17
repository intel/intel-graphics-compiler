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

#
#
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(OpenCLPasses)


add_subdirectory(IGCInstCombiner)

set(IGC_BUILD__SRC__Optimizer
    "${CMAKE_CURRENT_SOURCE_DIR}/BuiltInFuncImport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeAssumption.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixFastMathFlags.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GatingSimilarSamples.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IndirectCallOptimization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InfiniteLoopRemoval.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivConstantReduction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemCombine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LinkMultiRateShaders.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MarkReadOnlyLoad.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MCSOptimization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIConverter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PreCompiledFuncImport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PreCompiledFuncLibrary.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RectListOptimizationPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Scalarizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SetMathPrecisionForPositionOutput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SynchronizationObjectCoalescing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ValueTracker.cpp"
  )

set(IGC_BUILD__SRC__Compiler_Optimizer
    ${IGC_BUILD__SRC__Optimizer}
    ${IGC_BUILD__SRC__Optimizer_OpenCLPasses}
    ${IGC_BUILD__SRC__Optimizer_IGCInstCombiner}
    PARENT_SCOPE
  )

set(IGC_BUILD__HDR__Optimizer
    "${CMAKE_CURRENT_SOURCE_DIR}/BuiltInFuncImport.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeAssumption.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FixFastMathFlags.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/GatingSimilarSamples.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IndirectCallOptimization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InfiniteLoopRemoval.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivConstantReduction.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemCombine.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LinkMultiRateShaders.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MCSOptimization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIConverter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PreCompiledFuncImport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RectListOptimizationPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Scalarizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SetMathPrecisionForPositionOutput.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SynchronizationObjectCoalescing.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ValueTracker.h"
  )

set(IGC_BUILD__HDR__Optimizer
    ${IGC_BUILD__HDR__Optimizer}
    "${CMAKE_CURRENT_SOURCE_DIR}/../Builtins/LibraryDPEmu.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Builtins/LibraryMiscEmu.hpp"
  )

set(IGC_BUILD__HDR__Compiler_Optimizer
    ${IGC_BUILD__HDR__Optimizer}
    ${IGC_BUILD__HDR__Optimizer_OpenCLPasses}
    ${IGC_BUILD__HDR__Optimizer_IGCInstCombiner}
    PARENT_SCOPE
  )


igc_sg_register(
    Compiler__Optimizer
    "Optimizer"
    GROUPS
      Compiler__Optimizer_OpenCLPasses
      Compiler__Optimizer_IGCInstCombiner
    FILES
      ${IGC_BUILD__SRC__Optimizer}
      ${IGC_BUILD__HDR__Optimizer}
  )
