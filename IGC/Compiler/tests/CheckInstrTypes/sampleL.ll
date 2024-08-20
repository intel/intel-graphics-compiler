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

%__2D_DIM_Resource = type opaque

define <4 x float> @test(float %lod, float %u, float %v, float %r, float %ai, %__2D_DIM_Resource addrspace(2621440)* %res, i32 addrspace(2752512)* %sampler) {
entry:
  %texels = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float %lod, float %u, float %v, float %r, float %ai, %__2D_DIM_Resource addrspace(2621440)* undef, %__2D_DIM_Resource addrspace(2621440)* %res, i32 addrspace(2752512)* %sampler, i32 1, i32 2, i32 3)
  ret <4 x float> %texels
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
; CHECK: psHasSideEffect: 0
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
; CHECK: numCall: 1
; CHECK: numBarrier: 0
; CHECK: numLoadStore: 0
; CHECK: numWaveIntrinsics: 0
; CHECK: numAtomics: 0
; CHECK: numTypedReadWrite: 0
; CHECK: numAllInsts: 2
; CHECK: sampleCmpToDiscardOptimizationSlot: 0
; CHECK: numSample: 1
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
; CHECK: num2DAccesses: 1
; CHECK: numSLMAccesses: 0

; Function Attrs: nounwind readnone
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621440__2D_DIM_Resource.p2621440__2D_DIM_Resource.p2752512i32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621440)*, %__2D_DIM_Resource addrspace(2621440)*, i32 addrspace(2752512)*, i32, i32, i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!1}

!1 = !{!"ModuleMD", !2}
!2 = !{!"enableSampleLptrToLdmsptrSample0", i1 true}

