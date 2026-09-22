;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s
;
; Check 6: Verify that external definitions must dominate destination blocks.
; This test creates a pattern where:
; - DstTrue and DstFalse both branch to intermediate_block
; - intermediate_block defines a value (%val)
; - intermediate_block conditionally branches to TrueBB/FalseBB
; - TrueBB/FalseBB use %val
; - TrueBB is only reachable via DstTrue -> intermediate -> TrueBB
; - FalseBB is only reachable via DstFalse -> intermediate -> FalseBB
;
; If merged, intermediate_block would be unreachable and %val would be replaced
; with poison. Check 6 should reject this candidate.

define i32 @check6_external_def_dominance(i1 %cond1, i1 %cond2, i32 %a, i32 %b) {
entry:
  br i1 %cond1, label %DstTrue, label %DstFalse

DstTrue:
  br label %intermediate_block

DstFalse:
  br label %intermediate_block

intermediate_block:
  ; Define %val here - only dominates via intermediate_block
  %val = add i32 %a, %b
  br i1 %cond2, label %TrueBB, label %FalseBB

TrueBB:
  ; Use external def %val
  %res_true = mul i32 %val, 2
  br label %end

FalseBB:
  ; Use external def %val
  %res_false = mul i32 %val, 3
  br label %end

end:
  %result = phi i32 [%res_true, %TrueBB], [%res_false, %FalseBB]
  ret i32 %result
}

; CHECK: DstTrue:
; CHECK: br label %intermediate_block
; CHECK: DstFalse:
; CHECK: br label %intermediate_block
; CHECK: intermediate_block:
; CHECK: %val = add i32 %a, %b
; CHECK: br i1 %cond2, label %TrueBB, label %FalseBB
; CHECK: TrueBB:
; CHECK: %res_true = mul i32 %val, 2
; CHECK: FalseBB:
; CHECK: %res_false = mul i32 %val, 3
; CHECK-NOT: poison
