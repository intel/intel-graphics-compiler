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
; CHECK: hasStorageBufferLoad: 1
; CHECK: hasStorageBufferStore: 1
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
; CHECK: hasTypedRead: 0
; CHECK: hasTypedwrite: 0
; CHECK: mayHaveIndirectOperands: 0
; CHECK: mayHaveIndirectResources: 1
; CHECK: hasUniformAssumptions: 0
; CHECK: sampleCmpToDiscardOptimizationPossible: 0
; CHECK: hasRuntimeValueVector: 0
; CHECK: hasDynamicGenericLoadStore: 0
; CHECK: hasUnmaskedRegion: 0
; CHECK: hasSLM: 0
; CHECK: numCall: 6
; CHECK: numBarrier: 0
; CHECK: numLoadStore: 2
; CHECK: numWaveIntrinsics: 0
; CHECK: numAtomics: 0
; CHECK: numTypedReadWrite: 0
; CHECK: numAllInsts: 17
; CHECK: sampleCmpToDiscardOptimizationSlot: 0
; CHECK: numSample: 0
; CHECK: numBB: 1
; CHECK: numLoopInsts: 0
; CHECK: numOfLoop: 0
; CHECK: numInsts: 17
; CHECK: numAllocaInsts: 0
; CHECK: numPsInputs: 0
; CHECK: hasPullBary: 0
; CHECK: numGlobalInsts: 0
; CHECK: numLocalInsts: 17
; CHECK: numSamplesVaryingResource: 0
; CHECK: numUntyped: 0
; CHECK: num1DAccesses: 6
; CHECK: num2DAccesses: 0
; CHECK: numSLMAccesses: 0

define void @test_vectorpro(<2 x i16> addrspace(1)* %src) {
  %1 = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src, i32 4, i32 2, i1 true)
  %2 = bitcast <2 x i16> %1 to i32
  call void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src, i32 0, <2 x i16> %1, i32 2, i1 true)
  %3 = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
  store i32 %2, i32 addrspace(1)* %3, align 4
  %4 = bitcast <2 x i16> %1 to float
  %5 = bitcast <2 x i16> addrspace(1)* %src to float addrspace(1)*
  store float %4, float addrspace(1)* %5, align 4
  ret void
}

declare <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, <2 x i16>, i32, i1)

define void @test_pointers(i32 %bindlessOffset){
  %1 = inttoptr i32 %bindlessOffset to i32 addrspace(1)* addrspace(2490368)*
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.ldraw.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)* %1, i32 160, i32 8, i1 false)
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368p1i32.p1i32(i32 addrspace(1)* addrspace(2490368)* %1, i32 4, i32 addrspace(1)* %2, i32 1, i1 false)
  ret void
}

declare i32 addrspace(1)* @llvm.genx.GenISA.ldraw.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368p1i32.p1i32(i32 addrspace(1)* addrspace(2490368)*, i32, i32 addrspace(1)*, i32, i1)

define void @test_vector_of_pointers(i32 %bindlessOffset) {
  %1 = inttoptr i32 %bindlessOffset to <4 x i32 addrspace(1)*> addrspace(2490368)*
  %2 = call <4 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v4p1i32.p2490368v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)* %1, i32 160, i32 8, i1 false)
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4p1i32.v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)* %1, i32 4, <4 x i32 addrspace(1)*> %2, i32 1, i1 false)
  ret void
}

declare <4 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v4p1i32.p2490368v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4p1i32.v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)*, i32, <4 x i32 addrspace(1)*>, i32, i1)
