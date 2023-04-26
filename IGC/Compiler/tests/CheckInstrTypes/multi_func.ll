;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --serialize-igc-metadata --CheckInstrTypes --enable-instrtypes-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CheckInstrTypes
; ------------------------------------------------

; Test checks metrics gathered by CheckInstrTypes pass

; CHECK: CorrelatedValuePropagationEnable: 0
; CHECK: hasMultipleBB: 0
; CHECK: hasCmp: 1
; CHECK: hasSwitch: 1
; CHECK: hasPhi: 1
; CHECK: hasLoadStore: 1
; CHECK: hasIndirectCall: 0
; CHECK: hasInlineAsm: 0
; CHECK: hasInlineAsmPointerAccess: 0
; CHECK: hasIndirectBranch: 1
; CHECK: hasFunctionAddressTaken: 0
; CHECK: hasSel: 1
; CHECK: hasPointer: 0
; CHECK: hasLocalLoadStore: 0
; CHECK: hasGlobalLoad: 1
; CHECK: hasGlobalStore: 1
; CHECK: hasStorageBufferLoad: 0
; CHECK: hasStorageBufferStore: 0
; CHECK: hasSubroutines: 1
; CHECK: hasPrimitiveAlloca: 0
; CHECK: hasNonPrimitiveAlloca: 1
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
; CHECK: mayHaveIndirectResources: 0
; CHECK: hasUniformAssumptions: 0
; CHECK: sampleCmpToDiscardOptimizationPossible: 0
; CHECK: hasRuntimeValueVector: 0
; CHECK: hasDynamicGenericLoadStore: 0
; CHECK: hasUnmaskedRegion: 0
; CHECK: numCall: 2
; CHECK: numBarrier: 0
; CHECK: numLoadStore: 4
; CHECK: numWaveIntrinsics: 0
; CHECK: numAtomics: 0
; CHECK: numTypedReadWrite: 0
; CHECK: numAllInsts: 24
; CHECK: sampleCmpToDiscardOptimizationSlot: 0
; CHECK: numSample: 0
; CHECK: numBB: 6
; CHECK: numLoopInsts: 7
; CHECK: numOfLoop: 1
; CHECK: numInsts: 24
; CHECK: numAllocaInsts: 1
; CHECK: numPsInputs: 0
; CHECK: hasPullBary: 0
; CHECK: numGlobalInsts: 2
; CHECK: numLocalInsts: 22

declare void @use_int(i32)

define void @test_func1(float %fp, i32 addrspace(1)* %glb) {
entry:
  %a = load i32, i32 addrspace(1)* %glb
  %b = add i32 %a, 1
  store i32 %b, i32 addrspace(1)* %glb

  %alo = alloca [256 x i32], align 4, addrspace(0)
  %ptr = getelementptr [256 x i32], [256 x i32] addrspace(0)* %alo, i32 %a, i32 0

  %sw = fcmp ogt float %fp, 1.0
  call void @test_func2(i32 %a, i32 %b, i32 addrspace(0)* %ptr, i1 %sw)

  ret void
}

define void @test_func2(i32 %a, i32 %b, i32 addrspace(0)* %ptr, i1 %sw) {
entry:
  switch i1 %sw, label %loop_end [ i1 0, label %plus_one
                                   i1 1, label %plus_zero ]

plus_one:
  %idx = add i32 %a, 1
  br label %plus_zero

plus_zero:
  %id = phi i32 [ %idx, %plus_one ], [ %a, %entry ]
  br label %loop_header

loop_header:
  indirectbr i8* blockaddress(@test_func2, %loop), [label %loop, label %loop_end]

loop:
  %indvar = phi i32 [ %id, %loop_header ], [ %nextindvar, %loop ]

  %cmp = icmp sle i32 %indvar, 0
  %sel = select i1 %cmp, i32 10, i32 20
  %nextindvar = sub i32 %sel, %indvar

  store i32 %indvar, i32 addrspace(0)* %ptr

  %cond = icmp ult i32 %nextindvar, %id
  br i1 %cond, label %loop, label %loop_end

loop_end:
  %ld = load i32, i32 addrspace(0)* %ptr
  call void @use_int(i32 %ld)
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !9}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (float, i32 addrspace(1)*)* @test_func1}
!3 = !{!"FuncMDValue[0]"}
!4 = !{!"FuncMDMap[1]", void (i32, i32, i32 addrspace(0)*, i1)* @test_func2}
!5 = !{!"FuncMDValue[1]"}
!6 = !{void (float, i32 addrspace(1)*)* @test_func1, !7}
!7 = !{!8}
!8 = !{!"function_type", i32 0}
!9 = !{void (i32, i32, i32 addrspace(0)*, i1)* @test_func2, !10}
!10 = !{!11}
!11 = !{!"function_type", i32 0}
