;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Profitability is decided on register pressure, not instruction count. Before a fold
; the two arms sit on mutually exclusive paths, so the peak live set over the region
; carries the LARGER arm's live-outs; after linearizing it carries BOTH. The delta is
; therefore the smaller of the two arms' live-out weights, which makes a diamond cost
; real registers and a triangle cost nothing -- on a triangle one side is the
; predecessor's own fallthrough, whose values are already live there.
;
; A budget of 0 pins the sign of the delta without depending on how WIAnalysis scores
; the uniformity of any particular value: only a fold with a delta of 0 or less gets
; through. The conditions are all derived from simdLaneId so the divergence gate is
; satisfied and pressure alone decides.

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxPressureDelta=1000 -S < %s 2>&1 | FileCheck %s --check-prefix=LOOSE
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxPressureDelta=1000 -S < %s 2>&1 | FileCheck %s --check-prefix=LOOSE
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxPressureDelta=0 -S < %s 2>&1 | FileCheck %s --check-prefix=TIGHT
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxPressureDelta=0 -S < %s 2>&1 | FileCheck %s --check-prefix=TIGHT

; A diamond: both arms are hoisted and both hand the merge a value of their own, so
; after linearizing the two live simultaneously. The delta is positive, so a zero
; budget refuses the fold while a loose one takes it. Nothing here is expensive --
; two adds -- which is the point: an instruction-count budget would wave this through.
define i32 @test_diamond_costs_pressure(i32 %a, i32 %b) {
; LOOSE-LABEL: define i32 @test_diamond_costs_pressure(
; LOOSE:         select i1 %c
; LOOSE-NOT:     br i1
;
; TIGHT-LABEL: define i32 @test_diamond_costs_pressure(
; TIGHT:         br i1 %c
; TIGHT:         phi i32
; TIGHT-NOT:     select
entry:
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %c = icmp ult i16 %l, 8
  br i1 %c, label %t, label %f

t:                                                ; preds = %entry
  %vt = add i32 %a, 1
  br label %merge

f:                                                ; preds = %entry
  %vf = add i32 %b, 2
  br label %merge

merge:                                            ; preds = %t, %f
  %r = phi i32 [ %vt, %t ], [ %vf, %f ]
  ret i32 %r
}

; The same two adds as a triangle: only one arm is hoisted, and the merge's other
; incoming comes over the predecessor's direct edge, so it is a value already live
; there. min(arm, 0) is 0, and the fold goes through even at a zero budget.
define i32 @test_triangle_is_free(i32 %a) {
; LOOSE-LABEL: define i32 @test_triangle_is_free(
; LOOSE:         select i1 %c
; LOOSE-NOT:     br i1
;
; TIGHT-LABEL: define i32 @test_triangle_is_free(
; TIGHT:         select i1 %c
; TIGHT-NOT:     br i1
entry:
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %c = icmp ult i16 %l, 8
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = add i32 %a, 1
  %w = add i32 %v, 2
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %w, %arm ], [ %a, %entry ]
  ret i32 %r
}

; A diamond carrying a second PHI whose two incomings are the same value. That PHI
; collapses with no select at all, so its weight is credited back and offsets the one
; arm-pair that does need a select -- the fold nets out to no added pressure and is
; taken even at a zero budget.
define i32 @test_select_free_phi_credited(i32 %a) {
; LOOSE-LABEL: define i32 @test_select_free_phi_credited(
; LOOSE:         select i1 %c
; LOOSE-NOT:     br i1
;
; TIGHT-LABEL: define i32 @test_select_free_phi_credited(
; TIGHT:         select i1 %c
; TIGHT-NOT:     br i1
entry:
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %c = icmp ult i16 %l, 8
  br i1 %c, label %t, label %f

t:                                                ; preds = %entry
  %vt = add i32 %a, 1
  br label %merge

f:                                                ; preds = %entry
  %vf = add i32 %a, 2
  br label %merge

merge:                                            ; preds = %t, %f
  %sel = phi i32 [ %vt, %t ], [ %vf, %f ]
  %same = phi i32 [ %a, %t ], [ %a, %f ]
  %r = add i32 %sel, %same
  ret i32 %r
}

declare i16 @llvm.genx.GenISA.simdLaneId() #0

attributes #0 = { nounwind readnone willreturn }

!igc.functions = !{!0, !1, !2}

!0 = !{ i32 (i32, i32)* @test_diamond_costs_pressure, !3}
!1 = !{ i32 (i32)* @test_triangle_is_free, !3}
!2 = !{ i32 (i32)* @test_select_free_phi_credited, !3}
!3 = !{!4}
!4 = !{!"function_type", i32 0}
