;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A region is linearized only if every instruction in the speculated arm is on the
; hoist allow-list. Memory, fdiv/frem, transcendentals and wide (double) FP are not on
; it, so isSpeculatable rejects them and the arm is refused outright -- no budget can
; buy them in -- while a purely cheap region still linearizes. The instruction-count
; backstop is pinned so the controls here are independent of default tuning.

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=40 -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedInsts=40 -S < %s 2>&1 | FileCheck %s

; A degeneracy guard around a reciprocal (fdiv): fdiv is not on the hoist
; allow-list, so the arm is not speculatable and the region is NOT linearized
; -- the branch, the guarded fdiv, and the PHI all survive and no select is
; formed. This is the shape that regressed SIMD32 when flattened.
define float @test_no_flatten_fdiv(i1 %c, float %a, float %b) {
; CHECK-LABEL: define float @test_no_flatten_fdiv(
; CHECK:         br i1 %c
; CHECK:         fdiv float
; CHECK:         phi float
; CHECK-NOT:     select
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %r = fdiv float %a, %b
  %s = fmul float %r, %r
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

; Transcendental intrinsics (sqrt) are not on the hoist allow-list -- only
; positively-vetted, cheap intrinsics are -- so isSpeculatable rejects them and the
; region is left branchy. Confirms the SAFE-BY-DEFAULT posture: an unrecognized
; intrinsic is never speculated.
define float @test_no_flatten_sqrt(i1 %c, float %a) {
; CHECK-LABEL: define float @test_no_flatten_sqrt(
; CHECK:         br i1 %c
; CHECK:         call float @llvm.sqrt.f32(float %a)
; CHECK:         phi float
; CHECK-NOT:     select
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %r = call float @llvm.sqrt.f32(float %a)
  %s = fadd float %r, 1.000000e+00
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

; Control: a purely cheap region (float mul/add, both allow-listed) is well inside the
; instruction backstop and adds no pressure, so it still linearizes to a select.
define float @test_flatten_cheap(i1 %c, float %a, float %b) {
; CHECK-LABEL: define float @test_flatten_cheap(
; CHECK:         fmul float %a, %b
; CHECK:         select i1 %c, float
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %m = fmul float %a, %b
  %s = fadd float %m, %a
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

; Wide-FP arithmetic is emulated/rate-limited, so a guarded double fmul is off the
; allow-list (only float/half/bfloat FP arithmetic is a single native op) and is
; rejected as illegal to speculate, leaving the region branchy -- even though the same
; fmul at fp32 folds (see test_flatten_cheap).
define double @test_no_flatten_double_fmul(i1 %c, double %a, double %b) {
; CHECK-LABEL: define double @test_no_flatten_double_fmul(
; CHECK:         br i1 %c
; CHECK:         fmul double
; CHECK:         phi double
; CHECK-NOT:     select
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %m = fmul double %a, %b
  %s = fadd double %m, %a
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi double [ %s, %then ], [ %a, %entry ]
  ret double %v
}

; Control: an fp32 fmul of the same shape is speculatable and still linearizes,
; confirming the wide-FP handling keys off type, not opcode.
define float @test_flatten_float_fmul(i1 %c, float %a, float %b) {
; CHECK-LABEL: define float @test_flatten_float_fmul(
; CHECK:         fmul float %a, %b
; CHECK:         select i1 %c, float
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %m = fmul float %a, %b
  %s = fadd float %m, %a
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

; Control: a cheap intrinsic on the allow-list (fabs) is speculatable and still
; linearizes -- the safe-by-default rule does not over-block recognized ops.
define float @test_flatten_cheap_intrinsic(i1 %c, float %a) {
; CHECK-LABEL: define float @test_flatten_cheap_intrinsic(
; CHECK:         call float @llvm.fabs.f32(float %a)
; CHECK:         select i1 %c, float
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %r = call float @llvm.fabs.f32(float %a)
  %s = fadd float %r, %a
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

; A load is a memory instruction and is not on the hoist allow-list, so the
; region is left branchy even when the pointer is dereferenceable: memory is
; never speculated, regardless of how cheap the surrounding region is.
define float @test_no_flatten_load(i1 %c, float* dereferenceable(4) %p, float %a) {
; CHECK-LABEL: define float @test_no_flatten_load(
; CHECK:         br i1 %c
; CHECK:         load float
; CHECK:         phi float
; CHECK-NOT:     select
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %r = load float, float* %p, align 4
  %s = fadd float %r, %a
  br label %merge

merge:                                            ; preds = %entry, %then
  %v = phi float [ %s, %then ], [ %a, %entry ]
  ret float %v
}

declare float @llvm.sqrt.f32(float)
declare float @llvm.fabs.f32(float)
