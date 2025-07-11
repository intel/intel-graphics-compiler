#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
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
    "${CMAKE_CURRENT_SOURCE_DIR}/GatingSimilarSamples.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IndirectCallOptimization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivConstantReduction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemCombine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MarkReadOnlyLoad.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MCSOptimization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIConverter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PreCompiledFuncImport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteToPredicatedMemoryAccess.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ReduceOptPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Scalarizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SynchronizationObjectCoalescing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ValueTracker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeValueVectorExtractPass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BarrierControlFlowOptimization.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WaveShuffleIndexSinking.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WaveAllJointReduction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemIncrementReduction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstructionHoistingOptimization.cpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/GatingSimilarSamples.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IndirectCallOptimization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivConstantReduction.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemCombine.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MCSOptimization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIConverter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/OCLBIUtils.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PreCompiledFuncImport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PromoteToPredicatedMemoryAccess.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ReduceOptPass.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Scalarizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SynchronizationObjectCoalescing.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ValueTracker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeValueVectorExtractPass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/BarrierControlFlowOptimization.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WaveShuffleIndexSinking.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WaveAllJointReduction.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/IntDivRemIncrementReduction.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/InstructionHoistingOptimization.hpp"
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
