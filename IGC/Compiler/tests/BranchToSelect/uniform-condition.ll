;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Only a divergent branch is linearized. A branch whose condition WIAnalysis proves
; work-item-uniform is a scalar jump that enters exactly one arm, so turning it into a
; select would make the not-taken arm's work unconditional -- strictly more instructions
; and register pressure. A divergent branch already enters both arms under a lane mask,
; so there the selects replace the mask setup and the reconvergence at no added cost.
;
; Uniformity here comes from RuntimeValue (a push constant, UNIFORM_GLOBAL) versus
; simdLaneId (lane identity, RANDOM). Both are on the speculation allow-list, so the
; arms themselves are foldable in every case below and only the condition differs.

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s --check-prefix=DIVONLY
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s --check-prefix=DIVONLY
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectDivergentOnly=0 -S < %s 2>&1 | FileCheck %s --check-prefix=ALL
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectDivergentOnly=0 -S < %s 2>&1 | FileCheck %s --check-prefix=ALL

; A uniform condition (compare of a push constant) is left branchy by default, and folds
; only once the gate is turned off -- which pins that the arm is otherwise foldable and
; that the condition alone is what refuses the fold.
define i32 @test_uniform_cond(i32 %a) {
; DIVONLY-LABEL: define i32 @test_uniform_cond(
; DIVONLY:         br i1 %c
; DIVONLY:         phi i32
; DIVONLY-NOT:     select
;
; ALL-LABEL:     define i32 @test_uniform_cond(
; ALL:             select i1 %c
; ALL-NOT:         br i1
entry:
  %rv = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
  %c = icmp sgt i32 %rv, 0
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = add i32 %a, 1
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; Control: the same region with a divergent condition (compare of the lane id) is
; linearized under either setting.
define i32 @test_divergent_cond(i32 %a) {
; DIVONLY-LABEL: define i32 @test_divergent_cond(
; DIVONLY:         select i1 %c
; DIVONLY-NOT:     br i1
;
; ALL-LABEL:     define i32 @test_divergent_cond(
; ALL:             select i1 %c
; ALL-NOT:         br i1
entry:
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %c = icmp ult i16 %l, 8
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = add i32 %a, 1
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; A diamond nested under a uniform branch: the gate is per-branch, not per-region. The
; inner divergent diamond folds and its merge block is absorbed into its predecessor,
; while the outer uniform branch keeps its control flow.
define i32 @test_divergent_inside_uniform(i32 %a, i32 %b) {
; DIVONLY-LABEL: define i32 @test_divergent_inside_uniform(
; DIVONLY:         br i1 %cu
; DIVONLY:         select i1 %cd
; DIVONLY-NOT:     br i1 %cd
entry:
  %rv = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
  %cu = icmp sgt i32 %rv, 0
  br i1 %cu, label %inner, label %exit

inner:                                            ; preds = %entry
  %l = call i16 @llvm.genx.GenISA.simdLaneId()
  %cd = icmp ult i16 %l, 8
  br i1 %cd, label %it, label %if

it:                                               ; preds = %inner
  %vt = add i32 %a, 1
  br label %ijoin

if:                                               ; preds = %inner
  %vf = add i32 %b, 2
  br label %ijoin

ijoin:                                            ; preds = %it, %if
  %iv = phi i32 [ %vt, %it ], [ %vf, %if ]
  br label %exit

exit:                                             ; preds = %entry, %ijoin
  %r = phi i32 [ %iv, %ijoin ], [ %a, %entry ]
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #0
declare i16 @llvm.genx.GenISA.simdLaneId() #0

attributes #0 = { nounwind readnone willreturn }

!igc.functions = !{!0, !1, !2}

!0 = !{ i32 (i32)* @test_uniform_cond, !3}
!1 = !{ i32 (i32)* @test_divergent_cond, !3}
!2 = !{ i32 (i32, i32)* @test_divergent_inside_uniform, !3}
!3 = !{!4}
!4 = !{!"function_type", i32 0}