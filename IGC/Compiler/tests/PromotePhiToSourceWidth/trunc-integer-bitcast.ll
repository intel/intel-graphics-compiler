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
; PromotePhiToSourceWidth - integer trunc + same-width bitcast (int->fp reinterpret)
; ------------------------------------------------
;
; Mirror of the fp->int repack case: the loop edge is bitcast(trunc(int)) where the
; truncated low bits hold a packed float. The pass widens the PHI to the trunc
; source width and sinks trunc+bitcast into the merge block. Bit-exact on the zero
; edge: bitcast(trunc(getNullValue(i32))) == bitcast(i16 0) == +0.0.

; i32 -> i16 -> half : widen PHI to i32, sink trunc + bitcast.
define void @trunc_bitcast_i32_half(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @trunc_bitcast_i32_half(
; CHECK:       A:
; CHECK-NEXT:    br label %M
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    [[BC:%.*]] = bitcast i16 [[T]] to half
; CHECK-NEXT:    store half [[BC]], ptr %p
; CHECK-NEXT:    ret void
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  %h = bitcast i16 %t to half
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %h, %A ], [ 0.0, %B ]
  store half %phi, ptr %p
  ret void
}

; i64 -> i32 -> float : a different width / float type exercises the same path.
define void @trunc_bitcast_i64_float(i1 %c, i64 %a, ptr %p) {
; CHECK-LABEL: @trunc_bitcast_i64_float(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i64 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i64 [[W]] to i32
; CHECK-NEXT:    [[BC:%.*]] = bitcast i32 [[T]] to float
; CHECK-NEXT:    store float [[BC]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i64 %a to i32
  %f = bitcast i32 %t to float
  br label %M
B:
  br label %M
M:
  %phi = phi float [ %f, %A ], [ 0.0, %B ]
  store float %phi, ptr %p
  ret void
}
