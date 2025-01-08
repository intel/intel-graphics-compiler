;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole --CheckInstrTypes -igc-serialize-metadata --enable-instrtypes-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CheckInstrTypes
; ------------------------------------------------

; Test checks metrics gathered by CheckInstrTypes pass

define void @test(ptr addrspace(3) %addr) {
entry:
  %dummy = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(ptr addrspace(3) %addr, ptr addrspace(3) %addr, i32 0, i32 9)
  ret void
}

; Test checks metrics gathered by CheckInstrTypes pass

; CHECK: CorrelatedValuePropagationEnable: 0
; CHECK: hasMultipleBB: 0
; CHECK: hasCmp: 0
; CHECK: hasSwitch: 0
; CHECK: hasPhi: 0
; CHECK: hasLoadStore: 0
; CHECK: hasIndirectCall: 0
; CHECK: hasInlineAsm: 0
; CHECK: hasInlineAsmPointerAccess: 0
; CHECK: hasIndirectBranch: 0
; CHECK: hasFunctionAddressTaken: 0
; CHECK: hasSel: 0
; CHECK: hasPointer: 0
; CHECK: hasLocalLoadStore: 0
; CHECK: hasGlobalLoad: 0
; CHECK: hasGlobalStore: 0
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
; CHECK: hasAtomics: 1
; CHECK: hasLocalAtomics: 1
; CHECK: hasDiscard: 0
; CHECK: hasTypedRead: 0
; CHECK: hasTypedwrite: 0
; CHECK: mayHaveIndirectOperands: 0
; CHECK: mayHaveIndirectResources: 0
; CHECK: hasUniformAssumptions: 0
; CHECK: sampleCmpToDiscardOptimizationPossible: 0
; CHECK: hasRuntimeValueVector: 0
; CHECK: hasDynamicGenericLoadStore: 0
; CHECK: hasUnmaskedRegion: 0
; CHECK: hasSLM: 0
; CHECK: numCall: 1
; CHECK: numBarrier: 0
; CHECK: numLoadStore: 0
; CHECK: numWaveIntrinsics: 0
; CHECK: numAtomics: 1
; CHECK: numTypedReadWrite: 0
; CHECK: numAllInsts: 2
; CHECK: sampleCmpToDiscardOptimizationSlot: 0
; CHECK: numSample: 0
; CHECK: numBB: 1
; CHECK: numLoopInsts: 0
; CHECK: numOfLoop: 0
; CHECK: numInsts: 2
; CHECK: numAllocaInsts: 0
; CHECK: numPsInputs: 0
; CHECK: hasPullBary: 0
; CHECK: numGlobalInsts: 0
; CHECK: numLocalInsts: 2
; CHECK: numSamplesVaryingResource: 0
; CHECK: numUntyped: 0
; CHECK: num1DAccesses: 0
; CHECK: num2DAccesses: 0
; CHECK: numSLMAccesses: 0

; Function Attrs: argmemonly nounwind
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(ptr addrspace(3), ptr addrspace(3), i32, i32) #2

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nounwind readnone }
attributes #2 = { argmemonly nounwind }

!IGCMetadata = !{!1}

!1 = !{!"ModuleMD"}
