#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
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
  CMTrans/GenXBTIAssignment.cpp
  CMTrans/GenXBIFFlagCtrlResolution.cpp
  CMTrans/GenXPrintfLegalization.cpp
  CMTrans/GenXPrintfResolution.cpp
  CMTrans/GenXImportOCLBiF.cpp
  CMTrans/GenXTranslateIntrinsics.cpp
  CMTrans/GenXTranslateSPIRVBuiltins.cpp
  CMTrans/GenXTrampolineInsertion.cpp
  CMTrans/GenXTypeLegalization.cpp
  CMTrans/GenXCloneIndirectFunctions.cpp
  CMTrans/GenXLinkageCorruptor.cpp
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
