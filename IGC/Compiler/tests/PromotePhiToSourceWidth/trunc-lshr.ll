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
; PromotePhiToSourceWidth - trunc(lshr(x, k)) packed-lane extraction
; ------------------------------------------------
;
; trunc(lshr(x, k)) selects a packed lane of a wide value. The pass carries the
; full wide value x in the promoted PHI and re-emits lshr+trunc in the merge
; block. Bit-exact on the zero guard: lshr(0, k) == 0, trunc(0) == 0.

; High 16-bit lane of an i32: widen PHI to i32, sink lshr + trunc.
define void @lshr_trunc_hi(i1 %c, i32 %x, ptr %p) {
; CHECK-LABEL: @lshr_trunc_hi(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %x, %A ], [ 0, %B ]
; CHECK-NEXT:    [[S:%.*]] = lshr i32 [[W]], 16
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[S]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %s = lshr i32 %x, 16
  %t = trunc i32 %s to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 0, %B ]
  store i16 %phi, ptr %p
  ret void
}

; Packed high f16 lane: trunc(lshr) followed by an int->fp bitcast. Widen to i32,
; sink lshr + trunc + bitcast.
define void @lshr_trunc_bitcast_half(i1 %c, i32 %x, ptr %p) {
; CHECK-LABEL: @lshr_trunc_bitcast_half(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %x, %A ], [ 0, %B ]
; CHECK-NEXT:    [[S:%.*]] = lshr i32 [[W]], 16
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[S]] to i16
; CHECK-NEXT:    [[BC:%.*]] = bitcast i16 [[T]] to half
; CHECK-NEXT:    store half [[BC]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %s = lshr i32 %x, 16
  %t = trunc i32 %s to i16
  %h = bitcast i16 %t to half
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %h, %A ], [ 0.0, %B ]
  store half %phi, ptr %p
  ret void
}
