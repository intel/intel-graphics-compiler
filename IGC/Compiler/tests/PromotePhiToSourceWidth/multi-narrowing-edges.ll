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
; PromotePhiToSourceWidth - multiple narrowing edges (consistent shape)
; ------------------------------------------------
;
; All other positive tests have a single narrowing edge. These exercise the
; multi-edge path: two incoming edges each carry a narrowing of the *same* shape
; (same source/result type, same fp/int kind, same optional bitcast), so the
; widened PHI takes the wide source from every narrowing edge and one shared
; narrowing is sunk into the merge block.

; Two integer trunc edges + a zero guard: widen to i32, sink one trunc.
define void @two_narrowing_int(i32 %s, i32 %a, i32 %b, ptr %p) {
; CHECK-LABEL: @two_narrowing_int(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ %b, %B ], [ 0, %Z ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  switch i32 %s, label %Z [ i32 0, label %A
                            i32 1, label %B ]
A:
  %ta = trunc i32 %a to i16
  br label %M
B:
  %tb = trunc i32 %b to i16
  br label %M
Z:
  br label %M
M:
  %phi = phi i16 [ %ta, %A ], [ %tb, %B ], [ 0, %Z ]
  store i16 %phi, ptr %p
  ret void
}

; Two fptrunc+bitcast edges + a zero guard: widen to float, sink fptrunc+bitcast.
define void @two_narrowing_fp_bitcast(i32 %s, float %a, float %b, ptr %p) {
; CHECK-LABEL: @two_narrowing_fp_bitcast(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi float [ %a, %A ], [ %b, %B ], [ 0.000000e+00, %Z ]
; CHECK-NEXT:    [[FT:%.*]] = fptrunc float [[W]] to half
; CHECK-NEXT:    [[BC:%.*]] = bitcast half [[FT]] to i16
; CHECK-NEXT:    store i16 [[BC]], ptr %p
entry:
  switch i32 %s, label %Z [ i32 0, label %A
                            i32 1, label %B ]
A:
  %fa = fptrunc float %a to half
  %ia = bitcast half %fa to i16
  br label %M
B:
  %fb = fptrunc float %b to half
  %ib = bitcast half %fb to i16
  br label %M
Z:
  br label %M
M:
  %phi = phi i16 [ %ia, %A ], [ %ib, %B ], [ 0, %Z ]
  store i16 %phi, ptr %p
  ret void
}

; Two integer trunc edges + a non-zero guard: the guard widens via zext (5),
; bit-exact for a plain integer trunc (trunc(zext 5) == 5). Exercises the
; multi-edge and non-zero-guard paths together.
define void @two_narrowing_nonzero_guard(i32 %s, i32 %a, i32 %b, ptr %p) {
; CHECK-LABEL: @two_narrowing_nonzero_guard(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ %b, %B ], [ 5, %Z ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  switch i32 %s, label %Z [ i32 0, label %A
                            i32 1, label %B ]
A:
  %ta = trunc i32 %a to i16
  br label %M
B:
  %tb = trunc i32 %b to i16
  br label %M
Z:
  br label %M
M:
  %phi = phi i16 [ %ta, %A ], [ %tb, %B ], [ 5, %Z ]
  store i16 %phi, ptr %p
  ret void
}
