;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformbmg --igc-sink-ptr-const-add -S < %s | FileCheck %s
; ------------------------------------------------
; SinkPointerConstAdd
; ------------------------------------------------
; A constant nested below the add/sub feeding an ext may only be sunk if no
; add/sub between it and the ext wraps once it is removed. Otherwise the sunk
; offset is wrong: for a = 0xFFFFFFFF, b = 1 the original
; zext((a -nuw 5) +nuw b) is 0xFFFFFFFB, but zext(a + b) - 5 is -5.

; Removing a subtracted constant raises the sum, which may wrap.
; CHECK-LABEL: @nested_zext_sub_under_add(
; CHECK-NEXT: %t = sub nuw i32 %a, 5
; CHECK-NEXT: %s = add nuw i32 %t, %b
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_sub_under_add(i64 %base, i32 %a, i32 %b) {
  %t = sub nuw i32 %a, 5
  %s = add nuw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing an added constant lowers the sum, which cannot wrap. nsw is not
; proven and is dropped: for a = 0x7FFFFFFB, b = 0x7FFFFFFF, t = 0x80000000
; (signed overflow, allowed without nsw) and s = -1 as signed, but a + b
; overflows.
; CHECK-LABEL: @nested_zext_add_under_add(
; CHECK-NEXT: %s = add nuw i32 %a, %b
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, 5
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_add_under_add(i64 %base, i32 %a, i32 %b) {
  %t = add nuw i32 %a, 5
  %s = add nuw nsw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing a subtracted constant from the minuend raises the difference, which
; stays <= %a.
; CHECK-LABEL: @nested_zext_sub_in_minuend(
; CHECK-NEXT: %s = sub nuw i32 %a, %b
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, -5
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_sub_in_minuend(i64 %base, i32 %a, i32 %b) {
  %t = sub nuw i32 %a, 5
  %s = sub nuw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing an added constant from the subtrahend raises the difference, which
; stays <= %z.
; CHECK-LABEL: @nested_zext_add_in_subtrahend(
; CHECK-NEXT: %s = sub nuw i32 %z, %a
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, -5
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_add_in_subtrahend(i64 %base, i32 %a, i32 %z) {
  %t = add nuw i32 %a, 5
  %s = sub nuw i32 %z, %t
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing an added constant from the minuend lowers the difference, which may
; wrap below zero.
; CHECK-LABEL: @nested_zext_add_in_minuend(
; CHECK-NEXT: %t = add nuw i32 %a, 5
; CHECK-NEXT: %s = sub nuw i32 %t, %b
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_add_in_minuend(i64 %base, i32 %a, i32 %b) {
  %t = add nuw i32 %a, 5
  %s = sub nuw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing a subtracted constant from the subtrahend lowers the difference,
; which may wrap below zero.
; CHECK-LABEL: @nested_zext_sub_in_subtrahend(
; CHECK-NEXT: %t = sub nuw i32 %a, 5
; CHECK-NEXT: %s = sub nuw i32 %z, %t
; CHECK-NEXT: %e = zext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_zext_sub_in_subtrahend(i64 %base, i32 %a, i32 %z) {
  %t = sub nuw i32 %a, 5
  %s = sub nuw i32 %z, %t
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; A signed sum may wrap in either direction. Removing +5 lowers it:
; a = INT_MIN, b = -1 gives t = INT_MIN + 5 and s = INT_MIN + 4, but a + b
; overflows.
; CHECK-LABEL: @nested_sext_add(
; CHECK-NEXT: %t = add nsw i32 %a, 5
; CHECK-NEXT: %s = add nsw i32 %t, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_add(i64 %base, i32 %a, i32 %b) {
  %t = add nsw i32 %a, 5
  %s = add nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing -5 raises it: a = INT_MAX, b = 1 gives t = INT_MAX - 5 and
; s = INT_MAX - 4, but a + b overflows.
; CHECK-LABEL: @nested_sext_add_neg(
; CHECK-NEXT: %t = add nsw i32 %a, -5
; CHECK-NEXT: %s = add nsw i32 %t, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_add_neg(i64 %base, i32 %a, i32 %b) {
  %t = add nsw i32 %a, -5
  %s = add nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; With nuw and nsw on every add, the flags alone prove that lowering the sum
; keeps it in signed range.
; CHECK-LABEL: @nested_sext_add_flags_proven(
; CHECK-NEXT: %s = add nuw nsw i32 %a, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, 5
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_add_flags_proven(i64 %base, i32 %a, i32 %b) {
  %t = add nuw nsw i32 %a, 5
  %s = add nuw nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; The constant 0xFFFFFFFF is -1 as signed. nuw forces a = b = 0, so s goes
; from -1 to 0: the sign flips, but only by a change of nearly 2^32 as
; unsigned value, which the flags still prove safe.
; CHECK-LABEL: @nested_sext_sign_flip_flags_proven(
; CHECK-NEXT: %s = add nuw nsw i32 %a, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, -1
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_sign_flip_flags_proven(i64 %base, i32 %a, i32 %b) {
  %t = add nuw nsw i32 %a, -1
  %s = add nuw nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; The constant 0xFFFFFFF0 is -16 as signed. nuw bounds a, b <= 15, so s goes
; from [-16, -1] to [0, 15]: the sign flips for every input, but only by a
; change of nearly 2^32 as unsigned value, which the flags still prove safe.
; CHECK-LABEL: @nested_sext_sign_flip_range_flags_proven(
; CHECK-NEXT: %s = add nuw nsw i32 %a, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, -16
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_sign_flip_range_flags_proven(i64 %base, i32 %a, i32 %b) {
  %t = add nuw nsw i32 %a, -16
  %s = add nuw nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Raising the sum is not proven by the flags: a = INT_MAX, b = 1 gives
; t = INT_MAX - 5 and s = INT_MAX - 4, but a + b overflows.
; CHECK-LABEL: @nested_sext_raised_sum_not_sunk(
; CHECK-NEXT: %t = sub nuw nsw i32 %a, 5
; CHECK-NEXT: %s = add nuw nsw i32 %t, %b
; CHECK-NEXT: %e = sext i32 %s to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %ptr = inttoptr i64 %p to ptr addrspace(1)
define ptr addrspace(1) @nested_sext_raised_sum_not_sunk(i64 %base, i32 %a, i32 %b) {
  %t = sub nuw nsw i32 %a, 5
  %s = add nuw nsw i32 %t, %b
  %e = sext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; A chain of direct constant operands is peeled one level at a time:
; zext((a +nuw 128) -nuw 32) == zext(a) + 96.
; CHECK-LABEL: @chained_zext(
; CHECK-NEXT: %e = zext i32 %a to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, 96
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @chained_zext(i64 %base, i32 %a) {
  %t = add nuw i32 %a, 128
  %s = sub nuw i32 %t, 32
  %e = zext i32 %s to i64
  %p = add nuw i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; No-wrap flags on i64 instructions whose operand lost a constant are dropped.
; nuw: a = 2^64 - 1, b = 1: t = 2^64 - 6, s = 2^64 - 5, but a + b wraps.
; nsw: a = INT64_MAX, b = 1: t = INT64_MAX - 5, s = INT64_MAX - 4, but a + b
; overflows.
; CHECK-LABEL: @drop_flags(
; CHECK-NEXT: %s = add i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, -5
; CHECK-NEXT: %{{[0-9]+}} = inttoptr i64 %[[SUNK]] to ptr addrspace(1)
define ptr addrspace(1) @drop_flags(i64 %a, i64 %b) {
  %t = add i64 %a, -5
  %s = add nuw nsw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}
