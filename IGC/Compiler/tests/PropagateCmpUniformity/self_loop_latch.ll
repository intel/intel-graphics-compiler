;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-propagate-cmp-uniformity -S < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: compare in a single-block loop whose true edge is the back edge
; %iv == %K holds on the back edge, but uses of %iv in the loop block run
; before the branch, so they must not be replaced.
; ============================================================================
; CHECK-LABEL: @test_self_loop_latch(
define spir_kernel void @test_self_loop_latch(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  br label %loop

loop:
; CHECK-LABEL: loop:
; CHECK: %gep = getelementptr inbounds float, ptr addrspace(1) %out, i32 %iv
; CHECK: %iv.next = add i32 %iv, 1
  %iv = phi i32 [ %tid, %entry ], [ %iv.next, %loop ]
  %gep = getelementptr inbounds float, ptr addrspace(1) %out, i32 %iv
  store float 1.000000e+00, ptr addrspace(1) %gep, align 4
  %iv.next = add i32 %iv, 1
  %cmp = icmp eq i32 %iv, %K
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_self_loop_latch, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_self_loop_latch}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
