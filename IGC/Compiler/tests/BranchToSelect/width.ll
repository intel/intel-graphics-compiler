;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s

; Speculation is limited to values that live in a single 32-bit register and to ops
; that lower to one native instruction. A value wider than 32 bits (i64, double) is
; never speculated -- it occupies two GRFs and legalizes into a multi-op sequence --
; and multiplies wider than 16 bits are rejected too (32-bit mul is mul+mach, 64-bit
; is emulated). Pointers are exempt: they are inherently 64-bit, but pointer plumbing
; (gep/ptrtoint/casts) is not emulated arithmetic, so it stays speculatable.

; Negative: an i64 integer add is wide -- not speculated, branch survives.
define i64 @test_no_speculate_i64_add(i1 %c, i64 %a, i64 %b) {
; CHECK-LABEL: define i64 @test_no_speculate_i64_add(
; CHECK:         br i1 %c
; CHECK:         add i64
; CHECK:         phi i64
; CHECK-NOT:     select
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = add i64 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i64 [ %v, %arm ], [ %a, %entry ]
  ret i64 %r
}

; Control: the same shape at i32 is native -- speculated to a select.
define i32 @test_speculate_i32_add(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_speculate_i32_add(
; CHECK:         add i32
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = add i32 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; Negative: a 32-bit multiply lowers to mul+mach (more than one native op) and is
; rejected, so the region stays branchy.
define i32 @test_no_speculate_i32_mul(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_no_speculate_i32_mul(
; CHECK:         br i1 %c
; CHECK:         mul i32
; CHECK:         phi i32
; CHECK-NOT:     select
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = mul i32 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i32 [ %v, %arm ], [ %a, %entry ]
  ret i32 %r
}

; Control: a <=16-bit multiply is a single native op and is speculated.
define i16 @test_speculate_i16_mul(i1 %c, i16 %a, i16 %b) {
; CHECK-LABEL: define i16 @test_speculate_i16_mul(
; CHECK:         mul i16
; CHECK:         select i1 %c
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = mul i16 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i16 [ %v, %arm ], [ %a, %entry ]
  ret i16 %r
}

; Negative: a 64-bit shift is an emulation sequence -- not speculated.
define i64 @test_no_speculate_i64_shl(i1 %c, i64 %a, i64 %b) {
; CHECK-LABEL: define i64 @test_no_speculate_i64_shl(
; CHECK:         br i1 %c
; CHECK:         shl i64
; CHECK:         phi i64
; CHECK-NOT:     select
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = shl i64 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i64 [ %v, %arm ], [ %a, %entry ]
  ret i64 %r
}

; Pointer exemption: ptrtoint yields an i64 but is pointer plumbing, not wide
; arithmetic, so it is speculated even though its result is 64-bit.
define i64 @test_speculate_ptrtoint(i1 %c, i32* %p, i64 %b) {
; CHECK-LABEL: define i64 @test_speculate_ptrtoint(
; CHECK:         ptrtoint
; CHECK:         select i1 %c, i64
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %pi = ptrtoint i32* %p to i64
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi i64 [ %pi, %arm ], [ %b, %entry ]
  ret i64 %r
}

; Negative: an int->FP conversion from a 64-bit integer legalizes into a sequence --
; not speculated.
define float @test_no_speculate_sitofp_i64(i1 %c, i64 %a, float %d) {
; CHECK-LABEL: define float @test_no_speculate_sitofp_i64(
; CHECK:         br i1 %c
; CHECK:         sitofp i64
; CHECK:         phi float
; CHECK-NOT:     select
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = sitofp i64 %a to float
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi float [ %v, %arm ], [ %d, %entry ]
  ret float %r
}

; Control: the same conversion from a native i32 is one op and is speculated.
define float @test_speculate_sitofp_i32(i1 %c, i32 %a, float %d) {
; CHECK-LABEL: define float @test_speculate_sitofp_i32(
; CHECK:         sitofp i32
; CHECK:         select i1 %c, float
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %arm, label %merge

arm:                                              ; preds = %entry
  %v = sitofp i32 %a to float
  br label %merge

merge:                                            ; preds = %entry, %arm
  %r = phi float [ %v, %arm ], [ %d, %entry ]
  ret float %r
}
