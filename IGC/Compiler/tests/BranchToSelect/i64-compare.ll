;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=3 -S < %s 2>&1 | FileCheck %s --check-prefix=CAP3
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=3 -S < %s 2>&1 | FileCheck %s --check-prefix=CAP3
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=4 -S < %s 2>&1 | FileCheck %s --check-prefix=CAP4
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=4 -S < %s 2>&1 | FileCheck %s --check-prefix=CAP4
; RUN: igc_opt --typed-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s --check-prefix=NEGATIVE
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s --check-prefix=NEGATIVE

; Scalar icmp eq i64 is the one wide compare that BranchToSelect may speculate.
; Its i64 emulation lowers to two i32 equalities and one i1 AND, so the compare
; costs three operations in the successor-size backstop. The following successor also
; contains an i1 OR: a budget of three rejects the four-operation successor, while a
; budget of four accepts it.
define i1 @test_i64_eq_cost(i1 %c, i64 %a, i64 %b, i1 %other) {
; CAP3-LABEL: define i1 @test_i64_eq_cost(
; CAP3:         br i1 %div
; CAP3:         %eq = icmp eq i64 %a, %b
; CAP3:         %or = or i1 %eq, %other
; CAP3:         phi i1
; CAP3-NEXT:    ret i1
;
; CAP4-LABEL: define i1 @test_i64_eq_cost(
; CAP4:         %eq = icmp eq i64 %a, %b
; CAP4:         %or = or i1 %eq, %other
; CAP4:         select i1 %div, i1 %or, i1 false
; CAP4-NEXT:    ret i1
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %div = icmp ult i16 %lane, 8
  br i1 %div, label %true_succ, label %merge

true_succ:                                         ; preds = %entry
  %eq = icmp eq i64 %a, %b
  %or = or i1 %eq, %other
  br label %merge

merge:                                            ; preds = %entry, %true_succ
  %r = phi i1 [ %or, %true_succ ], [ false, %entry ]
  ret i1 %r
}

; A different i64 predicate remains branchy. The three-operation charge is only
; valid for equality, whose emulation has the fixed two-compare-plus-AND shape.
define i1 @test_i64_ne_not_speculated(i1 %c, i64 %a, i64 %b) {
; NEGATIVE-LABEL: define i1 @test_i64_ne_not_speculated(
; NEGATIVE:       br i1 %div
; NEGATIVE:       %cmp = icmp ne i64 %a, %b
; NEGATIVE:       phi i1
; NEGATIVE-NOT:   select
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %div = icmp ult i16 %lane, 8
  br i1 %div, label %true_succ, label %merge

true_succ:                                         ; preds = %entry
  %cmp = icmp ne i64 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %true_succ
  %r = phi i1 [ %cmp, %true_succ ], [ false, %entry ]
  ret i1 %r
}

; Vector i64 equality is not the scalar fixed-cost case.
define <2 x i1> @test_vector_i64_eq_not_speculated(i1 %c, <2 x i64> %a, <2 x i64> %b) {
; NEGATIVE-LABEL: define <2 x i1> @test_vector_i64_eq_not_speculated(
; NEGATIVE:       br i1 %div
; NEGATIVE:       %cmp = icmp eq <2 x i64> %a, %b
; NEGATIVE:       phi <2 x i1>
; NEGATIVE-NOT:   select
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %div = icmp ult i16 %lane, 8
  br i1 %div, label %true_succ, label %merge

true_succ:                                         ; preds = %entry
  %cmp = icmp eq <2 x i64> %a, %b
  br label %merge

merge:                                            ; preds = %entry, %true_succ
  %r = phi <2 x i1> [ %cmp, %true_succ ], [ zeroinitializer, %entry ]
  ret <2 x i1> %r
}

; Other wide integer comparisons remain rejected.
define i1 @test_i128_eq_not_speculated(i1 %c, i128 %a, i128 %b) {
; NEGATIVE-LABEL: define i1 @test_i128_eq_not_speculated(
; NEGATIVE:       br i1 %div
; NEGATIVE:       %cmp = icmp eq i128 %a, %b
; NEGATIVE:       phi i1
; NEGATIVE-NOT:   select
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %div = icmp ult i16 %lane, 8
  br i1 %div, label %true_succ, label %merge

true_succ:                                         ; preds = %entry
  %cmp = icmp eq i128 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %true_succ
  %r = phi i1 [ %cmp, %true_succ ], [ false, %entry ]
  ret i1 %r
}

; f64 comparison remains rejected because its native/emulated cost depends on the
; target and predicate semantics.
define i1 @test_f64_eq_not_speculated(i1 %c, double %a, double %b) {
; NEGATIVE-LABEL: define i1 @test_f64_eq_not_speculated(
; NEGATIVE:       br i1 %div
; NEGATIVE:       %cmp = fcmp oeq double %a, %b
; NEGATIVE:       phi i1
; NEGATIVE-NOT:   select
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %div = icmp ult i16 %lane, 8
  br i1 %div, label %true_succ, label %merge

true_succ:                                         ; preds = %entry
  %cmp = fcmp oeq double %a, %b
  br label %merge

merge:                                            ; preds = %entry, %true_succ
  %r = phi i1 [ %cmp, %true_succ ], [ false, %entry ]
  ret i1 %r
}

!igc.functions = !{!0, !1, !2, !3, !4}
!0 = !{i1 (i1, i64, i64, i1)* @test_i64_eq_cost, !5}
!1 = !{i1 (i1, i64, i64)* @test_i64_ne_not_speculated, !5}
!2 = !{<2 x i1> (i1, <2 x i64>, <2 x i64>)* @test_vector_i64_eq_not_speculated, !5}
!3 = !{i1 (i1, i128, i128)* @test_i128_eq_not_speculated, !5}
!4 = !{i1 (i1, double, double)* @test_f64_eq_not_speculated, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}

declare i16 @llvm.genx.GenISA.simdLaneId() #0

attributes #0 = { nounwind readnone willreturn }
