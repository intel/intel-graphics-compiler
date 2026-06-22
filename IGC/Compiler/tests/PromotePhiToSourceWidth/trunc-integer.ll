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
; PromotePhiToSourceWidth - integer trunc coverage (no bitcast)
; ------------------------------------------------

; i32 -> i16 : widen PHI to i32, sink trunc.
define void @trunc_i32_i16(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @trunc_i32_i16(
; CHECK:       A:
; CHECK-NEXT:    br label %M
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ]
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
  %phi = phi i16 [ %t, %A ], [ 0, %B ]
  store i16 %phi, ptr %p
  ret void
}

; i32 -> i8 : widen PHI to i32, sink trunc.
define void @trunc_i32_i8(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @trunc_i32_i8(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i8
; CHECK-NEXT:    store i8 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i8
  br label %M
B:
  br label %M
M:
  %phi = phi i8 [ %t, %A ], [ 0, %B ]
  store i8 %phi, ptr %p
  ret void
}

; i16 -> i8 : widen PHI to i16, sink trunc.
define void @trunc_i16_i8(i1 %c, i16 %a, ptr %p) {
; CHECK-LABEL: @trunc_i16_i8(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i16 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i16 [[W]] to i8
; CHECK-NEXT:    store i8 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i16 %a to i8
  br label %M
B:
  br label %M
M:
  %phi = phi i8 [ %t, %A ], [ 0, %B ]
  store i8 %phi, ptr %p
  ret void
}
