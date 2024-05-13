#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(OPT_SOURCES
  CMAnalysis/InstructionSimplifyGenX.cpp
  CMAnalysis/ConstantFoldingGenX.cpp
  CMTrans/CMABI.cpp
  CMTrans/CMImpParam.cpp
  CMTrans/CMKernelArgOffset.cpp
  CMTrans/GenXBIFFlagCtrlResolution.cpp
  CMTrans/GenXBTIAssignment.cpp
  CMTrans/GenXCloneIndirectFunctions.cpp
  CMTrans/GenXFoldReduction.cpp
  CMTrans/GenXImportOCLBiF.cpp
  CMTrans/GenXLinkageCorruptor.cpp
  CMTrans/GenXPrintfLegalization.cpp
  CMTrans/GenXPrintfResolution.cpp
  CMTrans/GenXTrampolineInsertion.cpp
  CMTrans/GenXTranslateIntrinsics.cpp
  CMTrans/GenXTranslateSPIRVBuiltins.cpp
  CMTrans/GenXTypeLegalization.cpp
  CMPacketize/GenXPacketize.cpp
  CMPacketize/PacketBuilder.cpp
  CMPacketize/PacketBuilder_math.cpp
  CMPacketize/PacketBuilder_mem.cpp
  CMPacketize/PacketBuilder_misc.cpp
)

add_library(VCTransforms OBJECT ${OPT_SOURCES})

igc_get_llvm_targets(LLVM_LIBS
  Analysis
  BitReader
  Core
  Linker
  Support
  TransformUtils
  ScalarOpts
  )

target_link_libraries(VCTransforms
  ${LLVM_LIBS}

  VCHeaders
  VCSupport
  VCGeneralUtils
  VCGenXUtils
  LLVMGenXIntrinsics
  VCInternalIntrinsics
  )
