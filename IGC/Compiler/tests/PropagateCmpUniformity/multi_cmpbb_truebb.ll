;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-propagate-cmp-uniformity -S < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: Multiple cmpBBs all branching to the same trueBB on equality
; (currently not optimized; potential future improvement).
;
; Both cmpBB_A and cmpBB_B test %tid == %K and jump to trueBB on equality,
; so every path reaching trueBB has the equality condition enforced. In theory
; uses of %tid dominated by trueBB could be replaced with %K. However the
; current implementation requires trueBB->getSinglePredecessor() == cmpBB,
; which fails here because trueBB has two predecessors. No replacement occurs.
; ============================================================================
; CHECK-LABEL: @test_multi_cmpbb_truebb(
define spir_kernel void @test_multi_cmpbb_truebb(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %flag_cond = icmp ne i32 %flag, 0
  br i1 %flag_cond, label %cmpBB_A, label %cmpBB_B

cmpBB_A:
  %cmp1 = icmp eq i32 %tid, %K
  br i1 %cmp1, label %trueBB, label %exitA

cmpBB_B:
  %cmp2 = icmp eq i32 %tid, %K
  br i1 %cmp2, label %trueBB, label %exitB

trueBB:
  ; Both predecessors proved %tid == %K, but getSinglePredecessor() returns
  ; nullptr so the pass conservatively skips replacement in dominated blocks.
  br label %useBB

useBB:
  ; TODO: could replace %tid with %K here once the pass is extended to handle
  ; the case where all predecessors of trueBB enforce the same equality.
; CHECK-LABEL: useBB:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

exitA:
  br label %exit

exitB:
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_multi_cmpbb_truebb, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_multi_cmpbb_truebb}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
