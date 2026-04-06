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
; Test: trueBranch is a join block with multiple predecessors - no replacement
; joinBlock is the trueBranch for (icmp eq %tid, %K), but it is also reachable
; from other_pred where equality is not guaranteed.
; Because trueBranch->getSinglePredecessor() != cmpBB, canReplaceUse must
; return false for all uses in blocks dominated by joinBlock (e.g. %dominated).
; Without the fix the DT check DT.dominates(joinBlock, dominated) = true
; would incorrectly allow replacing %tid with %K in %dominated.
; ============================================================================
; CHECK-LABEL: @test_multientry_truebranch(
define spir_kernel void @test_multientry_truebranch(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %flag_cond = icmp ne i32 %flag, 0
  br i1 %flag_cond, label %other_pred, label %cmpBB

other_pred:
  ; Reaches joinBlock without proving %tid == %K.
  br label %joinBlock

cmpBB:
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %joinBlock, label %falseBranch

joinBlock:
  ; preds: other_pred (no equality) and cmpBB (equality proven on true edge).
  ; getSinglePredecessor() returns nullptr, so no replacement is permitted.
  br label %dominated

dominated:
  ; Only predecessor is joinBlock, so DT.dominates(joinBlock, dominated) = true.
  ; The fix prevents replacement here because joinBlock has multiple predecessors.
; CHECK-LABEL: dominated:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
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
!1 = !{ptr @test_multientry_truebranch, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_multientry_truebranch}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
