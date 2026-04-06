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
; Test: PHI in trueBranch where trueBranch has only cmpBB as predecessor
; (valid special case). falseBranch goes elsewhere and is not a predecessor of
; trueBranch, so the PHI incoming from cmpBB is replaced. The domination
; direction (trueBranch does not dominate cmpBB) would prevent the main code
; path from firing, so this exercises the special PHI case specifically.
; ============================================================================
; CHECK-LABEL: @test_phi_incoming_single_pred(
define spir_kernel void @test_phi_incoming_single_pred(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
  ; Only predecessor is entry (= cmpBB). All lanes here had %tid == %K,
  ; so the PHI incoming from entry is safely replaced with %K.
; CHECK-LABEL: trueBranch:
; CHECK: %phi_tid = phi i32 [ %K, %entry ]
  %phi_tid = phi i32 [ %tid, %entry ]
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
!1 = !{ptr @test_phi_incoming_single_pred, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_phi_incoming_single_pred}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
