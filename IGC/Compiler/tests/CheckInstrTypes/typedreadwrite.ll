;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --CheckInstrTypes -igc-serialize-metadata --enable-instrtypes-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CheckInstrTypes
; ------------------------------------------------

; Test checks metrics gathered by CheckInstrTypes pass

; CHECK: CorrelatedValuePropagationEnable: 0
; CHECK: hasMultipleBB: 0
; CHECK: hasCmp: 0
; CHECK: hasSwitch: 0
; CHECK: hasPhi: 0
; CHECK: hasLoadStore: 1
; CHECK: hasIndirectCall: 0
; CHECK: hasInlineAsm: 0
; CHECK: hasInlineAsmPointerAccess: 0
; CHECK: hasIndirectBranch: 0
; CHECK: hasFunctionAddressTaken: 0
; CHECK: hasSel: 0
; CHECK: hasPointer: 0
; CHECK: hasLocalLoadStore: 0
; CHECK: hasGlobalLoad: 0
; CHECK: hasGlobalStore: 1
; CHECK: hasStorageBufferLoad: 0
; CHECK: hasStorageBufferStore: 0
; CHECK: hasSubroutines: 0
; CHECK: hasPrimitiveAlloca: 0
; CHECK: hasNonPrimitiveAlloca: 0
; CHECK: hasReadOnlyArray: 0
; CHECK: hasBuiltin: 0
; CHECK: hasFRem: 0
; CHECK: psHasSideEffect: 1
; CHECK: hasGenericAddressSpacePointers: 0
; CHECK: hasDebugInfo: 0
; CHECK: hasAtomics: 0
; CHECK: hasDiscard: 0
; CHECK: hasTypedRead: 1
; CHECK: hasTypedwrite: 1
; CHECK: mayHaveIndirectOperands: 0
; CHECK: mayHaveIndirectResources: 1
; CHECK: hasUniformAssumptions: 0
; CHECK: sampleCmpToDiscardOptimizationPossible: 0
; CHECK: hasRuntimeValueVector: 0
; CHECK: hasDynamicGenericLoadStore: 0
; CHECK: hasUnmaskedRegion: 0
; CHECK: hasSLM: 0
; CHECK: numCall: 4
; CHECK: numBarrier: 0
; CHECK: numLoadStore: 1
; CHECK: numWaveIntrinsics: 0
; CHECK: numAtomics: 0
; CHECK: numTypedReadWrite: 2
; CHECK: numAllInsts: 7
; CHECK: sampleCmpToDiscardOptimizationSlot: 0
; CHECK: numSample: 0
; CHECK: numBB: 1
; CHECK: numLoopInsts: 0
; CHECK: numOfLoop: 0
; CHECK: numInsts: 7
; CHECK: numAllocaInsts: 0
; CHECK: numPsInputs: 0
; CHECK: hasPullBary: 0
; CHECK: numGlobalInsts: 0
; CHECK: numLocalInsts: 7
; CHECK: numSamplesVaryingResource: 0
; CHECK: numUntyped: 0
; CHECK: num1DAccesses: 0
; CHECK: num2DAccesses: 2
; CHECK: numSLMAccesses: 0

define spir_kernel void @typedread(<4 x float> addrspace(1)* %res4f) {
entry:
  %ptr = call i32 addrspace(2228224)* @llvm.genx.GenISA.GetBufferPtr.p2228224i32(i32 1, i32 2)
  %typed.read = call <4 x float> @llvm.genx.GenISA.typedread.p2228224i32(i32 addrspace(2228224)* %ptr, i32 1, i32 2, i32 3, i32 4)
  store <4 x float> %typed.read, <4 x float> addrspace(1)* %res4f
  ret void
}

define spir_kernel void @typedwrite() {
entry:
  %ptr2 = call i32 addrspace(2228228)* @llvm.genx.GenISA.GetBufferPtr.p2228228i32(i32 1, i32 2)
  call void @llvm.genx.GenISA.typedwrite.p2228228i32(i32 addrspace(2228228)* %ptr2, i32 1, i32 2, i32 3, i32 4, float 5.0, float 6.0, float 7.0, float 8.0)
  ret void
}

declare <4 x float> @llvm.genx.GenISA.typedread.p2228224i32(i32 addrspace(2228224)*, i32, i32, i32, i32)
declare i32 addrspace(2228224)* @llvm.genx.GenISA.GetBufferPtr.p2228224i32(i32, i32)
declare void @llvm.genx.GenISA.typedwrite.p2228228i32(i32 addrspace(2228228)*, i32, i32, i32, i32, float, float, float, float)
declare i32 addrspace(2228228)* @llvm.genx.GenISA.GetBufferPtr.p2228228i32(i32, i32)
