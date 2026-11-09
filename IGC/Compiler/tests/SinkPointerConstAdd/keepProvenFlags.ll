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
; No-wrap flags on instructions above a removed constant are kept only where
; they are still proven: the instruction and everything below it up to the
; constant carry nuw, and the change lowers a sum or raises a difference. nsw
; is kept if nuw is kept and every such instruction also carries nsw.

; ---- i64 chain ----

; CHECK-LABEL: @i64_add_keeps_nuw(
; CHECK-NEXT: %s = add nuw i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, 5
define ptr addrspace(1) @i64_add_keeps_nuw(i64 %a, i64 %b) {
  %t = add nuw i64 %a, 5
  %s = add nuw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; CHECK-LABEL: @i64_add_keeps_nuw_nsw(
; CHECK-NEXT: %s = add nuw nsw i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, 5
define ptr addrspace(1) @i64_add_keeps_nuw_nsw(i64 %a, i64 %b) {
  %t = add nuw nsw i64 %a, 5
  %s = add nuw nsw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing an added constant from the subtrahend raises the difference.
; CHECK-LABEL: @i64_subtrahend_keeps_nuw_nsw(
; CHECK-NEXT: %s = sub nuw nsw i64 %z, %a
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, -5
define ptr addrspace(1) @i64_subtrahend_keeps_nuw_nsw(i64 %a, i64 %z) {
  %t = add nuw nsw i64 %a, 5
  %s = sub nuw nsw i64 %z, %t
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; nsw alone is never kept. a = INT64_MIN, b = -1: t = INT64_MIN + 5 and
; s = INT64_MIN + 4 do not overflow, but a + b does.
; CHECK-LABEL: @i64_nsw_only_dropped(
; CHECK-NEXT: %s = add i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, 5
define ptr addrspace(1) @i64_nsw_only_dropped(i64 %a, i64 %b) {
  %t = add nsw i64 %a, 5
  %s = add nsw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Removing a subtracted constant raises the sum, which may wrap.
; nuw: a = 2^64 - 1, b = 5: t = 2^64 - 6, s = 2^64 - 1, but a + b wraps.
; nsw: a = INT64_MAX, b = 1: t = INT64_MAX - 5, s = INT64_MAX - 4, but a + b
; overflows.
; CHECK-LABEL: @i64_raised_sum_dropped(
; CHECK-NEXT: %s = add i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, -5
define ptr addrspace(1) @i64_raised_sum_dropped(i64 %a, i64 %b) {
  %t = sub nuw nsw i64 %a, 5
  %s = add nuw nsw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; The leaf carries no flags, so nothing above it is proven.
; nuw: a = 2^64 - 5, b = 5: t wraps to 0 (allowed), s = 5, but a + b wraps.
; nsw: a = INT64_MAX - 4, b = 5: t overflows to INT64_MIN (allowed),
; s = INT64_MIN + 5, but a + b overflows.
; CHECK-LABEL: @i64_unflagged_leaf_dropped(
; CHECK-NEXT: %s = add i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, 5
define ptr addrspace(1) @i64_unflagged_leaf_dropped(i64 %a, i64 %b) {
  %t = add i64 %a, 5
  %s = add nuw nsw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; The direction of the change is that of the unsigned value, not the sign of
; the sunk offset: t = a +nuw (2^64 - 1) forces a = 0, so removing the constant
; lowers the minuend by 2^64 - 1, although the offset is -1. a = 0, b = 1:
; t = 2^64 - 1, s = 2^64 - 2, but a - b wraps.
; CHECK-LABEL: @i64_huge_constant_lowers_minuend(
; CHECK-NEXT: %s = sub i64 %a, %b
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, -1
define ptr addrspace(1) @i64_huge_constant_lowers_minuend(i64 %a, i64 %b) {
  %t = add nuw i64 %a, -1
  %s = sub nuw i64 %t, %b
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; Same constant in the subtrahend: lowering it raises the difference.
; CHECK-LABEL: @i64_huge_constant_lowers_subtrahend(
; CHECK-NEXT: %s = sub nuw i64 %z, %a
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %s, 1
define ptr addrspace(1) @i64_huge_constant_lowers_subtrahend(i64 %a, i64 %z) {
  %t = add nuw i64 %a, -1
  %s = sub nuw i64 %z, %t
  %ptr = inttoptr i64 %s to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; A zext lowered by its sunk constant changes exactly, both as unsigned and as
; signed value.
; CHECK-LABEL: @i64_above_zext_keeps_nuw_nsw(
; CHECK-NEXT: %e = zext i32 %a to i64
; CHECK-NEXT: %p = add nuw nsw i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, 5
define ptr addrspace(1) @i64_above_zext_keeps_nuw_nsw(i64 %base, i32 %a) {
  %t = add nuw i32 %a, 5
  %e = zext i32 %t to i64
  %p = add nuw nsw i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; A sext may move from -1 to 0, which is a large unsigned decrease.
; nuw: a = -1, base = 2^64 - 1: e = 0, p = 2^64 - 1, but base + sext(a) wraps.
; nsw: a = -1, base = INT64_MIN: e = 0, p = INT64_MIN, but base + sext(a)
; overflows.
; CHECK-LABEL: @i64_above_sext_dropped(
; CHECK-NEXT: %e = sext i32 %a to i64
; CHECK-NEXT: %p = add i64 %base, %e
; CHECK-NEXT: %[[SUNK:[0-9]+]] = add i64 %p, 1
define ptr addrspace(1) @i64_above_sext_dropped(i64 %base, i32 %a) {
  %t = add nsw i32 %a, 1
  %e = sext i32 %t to i64
  %p = add nuw nsw i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; ---- below a zext ----

; CHECK-LABEL: @zext_add_keeps_nsw(
; CHECK-NEXT: %s = add nuw nsw i32 %a, %b
; CHECK-NEXT: %e = zext i32 %s to i64
define ptr addrspace(1) @zext_add_keeps_nsw(i64 %base, i32 %a, i32 %b) {
  %t = add nuw nsw i32 %a, 5
  %s = add nuw nsw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; CHECK-LABEL: @zext_subtrahend_keeps_nsw(
; CHECK-NEXT: %s = sub nuw nsw i32 %z, %a
; CHECK-NEXT: %e = zext i32 %s to i64
define ptr addrspace(1) @zext_subtrahend_keeps_nsw(i64 %base, i32 %a, i32 %z) {
  %t = add nuw nsw i32 %a, 5
  %s = sub nuw nsw i32 %z, %t
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}

; The leaf has no nsw. a = 0x7FFFFFFB, b = 0x7FFFFFFF: t = 0x80000000
; (signed overflow, allowed), s = t + b = -1 as signed, but a + b overflows.
; CHECK-LABEL: @zext_unflagged_leaf_drops_nsw(
; CHECK-NEXT: %s = add nuw i32 %a, %b
; CHECK-NEXT: %e = zext i32 %s to i64
define ptr addrspace(1) @zext_unflagged_leaf_drops_nsw(i64 %base, i32 %a, i32 %b) {
  %t = add nuw i32 %a, 5
  %s = add nuw nsw i32 %t, %b
  %e = zext i32 %s to i64
  %p = add i64 %base, %e
  %ptr = inttoptr i64 %p to ptr addrspace(1)
  ret ptr addrspace(1) %ptr
}
