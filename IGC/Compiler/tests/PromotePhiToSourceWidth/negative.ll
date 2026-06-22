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
; PromotePhiToSourceWidth - negative cases: PHI must be left untouched
; ------------------------------------------------

; Non-zero guard with an lshr offset: the wide preimage is not a plain zext
; (it would be C << k), so the pass must bail. (A plain integer trunc with a
; non-zero guard IS handled - see nonzero-const-guard.ll.)
define void @nonzero_const_shifted(i1 %c, i32 %x, ptr %p) {
; CHECK-LABEL: @nonzero_const_shifted(
; CHECK:       M:
; CHECK-NEXT:    %phi = phi i16 [ %t, %A ], [ 5, %B ]
; CHECK-NOT:     phi i32
entry:
  br i1 %c, label %A, label %B
A:
  %s = lshr i32 %x, 16
  %t = trunc i32 %s to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 5, %B ]
  store i16 %phi, ptr %p
  ret void
}

; Non-zero guard with a bitcast: the wide preimage is not a plain zext, so bail.
; 0xH3C00 is 1.0 in half (non-null).
define void @nonzero_const_bitcast(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @nonzero_const_bitcast(
; CHECK:       M:
; CHECK-NEXT:    %phi = phi half [ %h, %A ], [ 0xH3C00, %B ]
; CHECK-NOT:     phi i32
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  %h = bitcast i16 %t to half
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %h, %A ], [ 0xH3C00, %B ]
  store half %phi, ptr %p
  ret void
}

; The narrowing chain has another user, so it cannot be relocated -> no transform.
define void @multiuse(i1 %c, float %a, ptr %p, ptr %q) {
; CHECK-LABEL: @multiuse(
; CHECK:       A:
; CHECK:         %i = bitcast half %t to i16
; CHECK:       M:
; CHECK-NEXT:    %phi = phi i16 [ %i, %A ], [ 0, %B ]
; CHECK-NOT:     phi float
entry:
  br i1 %c, label %A, label %B
A:
  %t = fptrunc float %a to half
  %i = bitcast half %t to i16
  store i16 %i, ptr %q
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %i, %A ], [ 0, %B ]
  store i16 %phi, ptr %p
  ret void
}

; Negative zero (0x8000 bits) is not getNullValue; widening to +0.0 would change
; the stored bits -> no transform.
define void @neg_zero(i1 %c, float %a, ptr %p) {
; CHECK-LABEL: @neg_zero(
; CHECK:       M:
; CHECK-NEXT:    %phi = phi half [ %t, %A ], [ 0xH8000, %B ]
; CHECK-NOT:     phi float
entry:
  br i1 %c, label %A, label %B
A:
  %t = fptrunc float %a to half
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %t, %A ], [ 0xH8000, %B ]
  store half %phi, ptr %p
  ret void
}

; A non-null, non-ConstantInt guard (poison) has no zext preimage -> bail.
define void @poison_guard(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @poison_guard(
; CHECK:       M:
; CHECK-NEXT:    %phi = phi i16 [ %t, %A ], [ poison, %B ]
; CHECK-NOT:     phi i32
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ poison, %B ]
  store i16 %phi, ptr %p
  ret void
}

; Two conv edges narrowing from different source widths -> inconsistent, bail.
define void @mixed_width(i1 %c, i32 %a, i64 %b, ptr %p) {
; CHECK-LABEL: @mixed_width(
; CHECK:       M:
; CHECK:         %phi = phi i16
; CHECK-NOT:     phi i32
; CHECK-NOT:     phi i64
entry:
  br i1 %c, label %A, label %B
A:
  %ta = trunc i32 %a to i16
  br label %M
B:
  %tb = trunc i64 %b to i16
  br label %M
M:
  %phi = phi i16 [ %ta, %A ], [ %tb, %B ]
  store i16 %phi, ptr %p
  ret void
}

; A bare narrowing (no bitcast) with a second use cannot be relocated -> bail.
; Complements @multiuse above, where the extra use is on the bitcast instead.
define void @narrowing_multiuse_no_bitcast(i1 %c, float %a, ptr %p, ptr %q) {
; CHECK-LABEL: @narrowing_multiuse_no_bitcast(
; CHECK:       A:
; CHECK:         %t = fptrunc float %a to half
; CHECK:       M:
; CHECK-NEXT:    %phi = phi half [ %t, %A ], [ 0xH0000, %B ]
; CHECK-NOT:     phi float
entry:
  br i1 %c, label %A, label %B
A:
  %t = fptrunc float %a to half
  store half %t, ptr %q
  br label %M
B:
  br label %M
M:
  %phi = phi half [ %t, %A ], [ 0.0, %B ]
  store half %phi, ptr %p
  ret void
}
