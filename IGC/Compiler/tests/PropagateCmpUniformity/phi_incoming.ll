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
; Test: PHI in multi-predecessor trueBranch - no replacement (mixed uniformity)
; trueBranch is reachable from both other_pred (non-uniform %tid) and cmpBB.
; Although equality holds on cmpBB->trueBranch, the other arm is non-uniform,
; so replacing the cmpBB arm with %K would leave the PHI non-uniform while
; introducing a SIMD broadcast on that path — net harm. PCU must skip it.
; ============================================================================
; CHECK-LABEL: @test_phi_incoming(
define spir_kernel void @test_phi_incoming(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %entry_cond = icmp ne i32 %flag, 0
  br i1 %entry_cond, label %other_pred, label %cmpBB

other_pred:
  br label %trueBranch

cmpBB:
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
; CHECK-LABEL: trueBranch:
; CHECK: %phi_tid = phi i32 [ %tid, %other_pred ], [ %tid, %cmpBB ]
  %phi_tid = phi i32 [ %tid, %other_pred ], [ %tid, %cmpBB ]
  %result = add i32 %phi_tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_phi_incoming, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_phi_incoming}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
