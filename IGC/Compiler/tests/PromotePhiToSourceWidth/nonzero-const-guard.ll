;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-promote-phi-to-source-width -S < %s | FileCheck %s
; ------------------------------------------------
; PromotePhiToSourceWidth - non-zero constant guard (integer trunc, via zext)
; ------------------------------------------------
;
; A non-zero constant on the bypass edge is widened with zext: trunc(zext C) == C
; is bit-exact for a plain integer trunc, so the guard need not be zero.

; Small constant guard.
define void @nonzero_int_guard(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @nonzero_int_guard(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 5, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 5, %B ]
  store i16 %phi, ptr %p
  ret void
}

; All-ones i16 guard widens via ZEXT (65535), not SEXT (-1 / 4294967295): proves
; the widened constant is zero-extended.
define void @nonzero_int_guard_allones(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @nonzero_int_guard_allones(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 65535, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ -1, %B ]
  store i16 %phi, ptr %p
  ret void
}
