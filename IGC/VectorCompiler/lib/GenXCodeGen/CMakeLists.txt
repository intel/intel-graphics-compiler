#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2020-2021 Intel Corporation
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

add_subdirectory(TargetInfo)
add_subdirectory(Utils)

set(LLVM_TARGET_DEFINITIONS GenX.td)
tablegen(LLVM GenXGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(GenXCommonTableGen)

set(GenXCommonTableGen_dependencies)
list(APPEND GenXCommonTableGen_dependencies
  GenXCommonTableGen)

add_custom_target(GenXCommonTableGen_target DEPENDS ${GenXCommonTableGen_dependencies})

set(CODEGEN_SOURCES
  FunctionGroup.cpp
  KillAnalysis.cpp
  GenXAddressCommoning.cpp
  GenXAggregatePseudoLowering.cpp
  GenXAlignmentInfo.cpp
  GenXAnalysisDumper.cpp
  GenXArgIndirection.cpp
  GenXBaling.cpp
  GenXCategory.cpp
  GenXCFSimplification.cpp
  GenXCisaBuilder.cpp
  GenXConstants.cpp
  GenXCoalescing.cpp
  GenXDeadVectorRemoval.cpp
  GenXDebugInfo.cpp
  GenXDepressurizer.cpp
  GenXExtractVectorizer.cpp
  GenXFuncPtrsLowering.cpp
  GenXGotoJoin.cpp
  GenXGEPLowering.cpp
  GenXGlobalVariableLowering.cpp
  GenXIMadPostLegalization.cpp
  GenXImportBiF.cpp
  GenXInlineAsmLowering.cpp
  GenXIntrinsics.cpp
  GenXLayoutBlocks.cpp
  GenXLegalization.cpp
  GenXLiveRanges.cpp
  GenXLiveness.cpp
  GenXLowering.cpp
  GenXLowerAggrCopies.cpp
  GenXLowerJmpTableSwitch.cpp
  GenXEmulate.cpp
  GenXModule.cpp
  GenXNumbering.cpp
  GenXOCLInfoExtractor.cpp
  GenXOCLRuntimeInfo.cpp
  GenXPatternMatch.cpp
  GenXPostLegalization.cpp
  GenXPrinter.cpp
  GenXPressureTracker.cpp
  GenXPrologEpilogInsertion.cpp
  GenXPromoteArray.cpp
  GenXThreadPrivateMemory.cpp
  GenXPromotePredicate.cpp
  GenXRawSendRipper.cpp
  GenXReduceIntSize.cpp
  GenXInstCombineCleanup.cpp
  GenXRegion.cpp
  GenXRegionCollapsing.cpp
  GenXRematerialization.cpp
  GenXSimdCFConformance.cpp
  GenXSubtarget.cpp
  GenXTargetMachine.cpp
  GenXTidyControlFlow.cpp
  GenXUnbaling.cpp
  GenXUtil.cpp
  GenXVectorDecomposer.cpp
  GenXVisaRegAlloc.cpp
  GenXVisa.cpp
)

add_library(VCCodeGen ${CODEGEN_SOURCES})
add_dependencies(VCCodeGen
  GenXUtilBuild
  GenXCommonTableGen_target
  )
target_include_directories(VCCodeGen
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  )
target_link_libraries(VCCodeGen
  LLVMGenXIntrinsics

  VCHeaders
  VCTransforms
  VCTargetInfo
  VCSupport
  GenXDebugInfo
  GenX_IR
  )
