;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-freeze-out-of-range-shifts -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Unknown amount: freeze the result. The shift keeps its flags - the freeze
; contains the poison, so there is no need to weaken the shift itself.
define i32 @shl_unknown_amount(i32 %x, i32 %amt) {
; CHECK-LABEL: @shl_unknown_amount(
; CHECK:         %r = shl nuw nsw i32 %x, %amt
; CHECK-NEXT:    %[[F:.*]] = freeze i32 %r
; CHECK-NEXT:    ret i32 %[[F]]
  %r = shl nuw nsw i32 %x, %amt
  ret i32 %r
}

; Narrow shift feeding a wider accumulator, with a truncated induction variable
; as the amount: in range on the first iterations, past the shift's bit width on
; the rest. Unfrozen, that poison reaches the loop exit branch and on LLVM 16+
; the loop is left with no exit at all.
define i64 @shl_truncated_iv() {
; CHECK-LABEL: @shl_truncated_iv(
; CHECK:         %shl = shl nuw i32 1, %trunc
; CHECK-NEXT:    %[[F:.*]] = freeze i32 %shl
; CHECK-NEXT:    %zext = zext i32 %[[F]] to i64
entry:
  br label %loop

loop:
  %acc = phi i64 [ 0, %entry ], [ %or, %loop ]
  %i = phi i64 [ 0, %entry ], [ %inc, %loop ]
  %trunc = trunc i64 %i to i32
  %shl = shl nuw i32 1, %trunc
  %zext = zext i32 %shl to i64
  %or = or i64 %acc, %zext
  %inc = add nuw nsw i64 %i, 2
  %cmp = icmp ult i64 %inc, 64
  br i1 %cmp, label %loop, label %exit

exit:
  ret i64 %or
}

define i32 @lshr_unknown_amount(i32 %x, i32 %amt) {
; CHECK-LABEL: @lshr_unknown_amount(
; CHECK:         %r = lshr exact i32 %x, %amt
; CHECK-NEXT:    %[[F:.*]] = freeze i32 %r
; CHECK-NEXT:    ret i32 %[[F]]
  %r = lshr exact i32 %x, %amt
  ret i32 %r
}

define i64 @ashr_unknown_amount(i64 %x, i64 %amt) {
; CHECK-LABEL: @ashr_unknown_amount(
; CHECK:         %r = ashr i64 %x, %amt
; CHECK-NEXT:    %[[F:.*]] = freeze i64 %r
; CHECK-NEXT:    ret i64 %[[F]]
  %r = ashr i64 %x, %amt
  ret i64 %r
}

; Already masked, provably in range, cannot be poison, no freeze.
define i32 @already_masked(i32 %x, i32 %amt) {
; CHECK-LABEL: @already_masked(
; CHECK-NOT:     freeze
  %m = and i32 %amt, 31
  %r = shl nuw i32 %x, %m
  ret i32 %r
}

; In-range constant, no freeze.
define i32 @constant_in_range(i32 %x) {
; CHECK-LABEL: @constant_in_range(
; CHECK-NOT:     freeze
  %r = shl nuw i32 %x, 5
  ret i32 %r
}

; Out-of-range constant. This is poison, so it gets frozen.
define i32 @constant_out_of_range(i32 %x) {
; CHECK-LABEL: @constant_out_of_range(
; CHECK:         %r = shl nuw i32 %x, 62
; CHECK-NEXT:    %[[F:.*]] = freeze i32 %r
; CHECK-NEXT:    ret i32 %[[F]]
  %r = shl nuw i32 %x, 62
  ret i32 %r
}

; zext i8 can reach 255, so not provably in range.
define i32 @zext_i8_amount(i32 %x, i8 %amt) {
; CHECK-LABEL: @zext_i8_amount(
; CHECK:         %r = shl i32 %x, %z
; CHECK-NEXT:    %[[F:.*]] = freeze i32 %r
  %z = zext i8 %amt to i32
  %r = shl i32 %x, %z
  ret i32 %r
}

; zext i4 cannot exceed 15. An i32 shift by it is always in range, no freeze.
define i32 @zext_i4_amount(i32 %x, i4 %amt) {
; CHECK-LABEL: @zext_i4_amount(
; CHECK-NOT:     freeze
  %z = zext i4 %amt to i32
  %r = shl i32 %x, %z
  ret i32 %r
}

; Handle vectors, the freeze covers the whole vector.
define <4 x i32> @shl_vector(<4 x i32> %x, <4 x i32> %amt) {
; CHECK-LABEL: @shl_vector(
; CHECK:         %r = shl <4 x i32> %x, %amt
; CHECK-NEXT:    %[[F:.*]] = freeze <4 x i32> %r
; CHECK-NEXT:    ret <4 x i32> %[[F]]
  %r = shl <4 x i32> %x, %amt
  ret <4 x i32> %r
}

; Check that freezing works for non-power-of-two widths.
define i24 @shl_i24(i24 %x, i24 %amt) {
; CHECK-LABEL: @shl_i24(
; CHECK:         %r = shl i24 %x, %amt
; CHECK-NEXT:    %[[F:.*]] = freeze i24 %r
; CHECK-NEXT:    ret i24 %[[F]]
  %r = shl i24 %x, %amt
  ret i24 %r
}

; A non-shift binary operator should not be handled.
define i32 @unrelated_binop(i32 %x, i32 %y) {
; CHECK-LABEL: @unrelated_binop(
; CHECK-NOT:     freeze
  %r = add i32 %x, %y
  ret i32 %r
}
