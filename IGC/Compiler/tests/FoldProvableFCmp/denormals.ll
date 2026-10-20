;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --regkey FoldProvableFCmpMarginBits=0 -igc-fold-provable-fcmp -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Folds that must not happen near zero, where FPRange snaps a denormal bound to
; zero. The margin is off: at the default it is wider than the denormal band and
; would hide an unsound snap rather than test it.
;
; Flushing preserves weak order and sign but destroys strictness, so the strict
; claims below must stay and the weak ones should still fold.
;
; ------------------------------------------------

declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
declare float @llvm.sqrt.f32(float)
declare void @use(i1)

; [1e-40, 5] snaps to [0, 5]. Under flush-to-zero x can be +0, so neither x > 0
; nor x != 0 holds. x >= 0 does, and is the control: if it stops folding the pass
; has gone inert and the rest proves nothing.
; CHECK-LABEL: define void @denorm_lower_bound(
; CHECK: fcmp nnan ogt
; CHECK: fcmp nnan one
; CHECK: call void @use(i1 true)
define void @denorm_lower_bound(float %v) {
  %hi = call float @llvm.minnum.f32(float %v, float 5.000000e+00)
  %x  = call float @llvm.maxnum.f32(float %hi, float 0x37A16C2000000000)
  %gt = fcmp nnan ogt float %x, 0.000000e+00
  %ne = fcmp nnan one float %x, 0.000000e+00
  %ge = fcmp nnan oge float %x, 0.000000e+00
  call void @use(i1 %gt)
  call void @use(i1 %ne)
  call void @use(i1 %ge)
  ret void
}

; Wholly inside the denormal band, every value flushes to zero: x > 0 is false
; when flushed and true when retained, x == 0 the reverse, so none may fold. The
; snap only moves the bound nearer zero, never both, for that reason.
; CHECK-LABEL: define void @wholly_denormal_band(
; CHECK: fcmp nnan ogt
; CHECK: fcmp nnan oeq
; CHECK: fcmp nnan olt
define void @wholly_denormal_band(float %v, float %w) {
  %ph = call float @llvm.minnum.f32(float %v, float 0x37A16C2000000000)
  %p  = call float @llvm.maxnum.f32(float %ph, float 0x37916C2000000000)
  %nh = call float @llvm.minnum.f32(float %w, float 0xB7916C2000000000)
  %n  = call float @llvm.maxnum.f32(float %nh, float 0xB7A16C2000000000)
  %c1 = fcmp nnan ogt float %p, 0.000000e+00
  %c2 = fcmp nnan oeq float %p, 0.000000e+00
  %c3 = fcmp nnan olt float %n, 0.000000e+00
  call void @use(i1 %c1)
  call void @use(i1 %c2)
  call void @use(i1 %c3)
  ret void
}

; The case the snap exists for: x is normal, x*x is entirely denormal and sqrt
; lifts it back out. Without the snap %len looks non-zero and the reciprocal
; finite, while the hardware divides by zero.
; CHECK-LABEL: define void @sqrt_launder(
; CHECK: fcmp nnan ogt
; CHECK: fcmp nnan olt
define void @sqrt_launder(float %v) {
  %hi  = call float @llvm.minnum.f32(float %v, float 0x3BC79CA100000000)
  %x   = call float @llvm.maxnum.f32(float %hi, float 0x3B5E392020000000)
  %xx  = fmul nnan float %x, %x
  %len = call float @llvm.sqrt.f32(float %xx)
  %c   = fcmp nnan ogt float %len, 0.000000e+00
  %rcp = fdiv nnan float 1.000000e+00, %len
  %c2  = fcmp nnan olt float %rcp, 1.000000e+10
  call void @use(i1 %c)
  call void @use(i1 %c2)
  ret void
}

; Two snapped ranges, [0, 5] against [-1e-40, -0]: both reach zero, so neither
; strict relation holds.
; CHECK-LABEL: define void @snapped_vs_snapped(
; CHECK: fcmp nnan ogt
; CHECK: fcmp nnan one
define void @snapped_vs_snapped(float %a, float %b) {
  %ah = call float @llvm.minnum.f32(float %a, float 5.000000e+00)
  %L  = call float @llvm.maxnum.f32(float %ah, float 0x37A16C2000000000)
  %bh = call float @llvm.minnum.f32(float %b, float 0xB7916C2000000000)
  %R  = call float @llvm.maxnum.f32(float %bh, float 0xB7A16C2000000000)
  %c1 = fcmp nnan ogt float %L, %R
  %c2 = fcmp nnan one float %L, %R
  call void @use(i1 %c1)
  call void @use(i1 %c2)
  ret void
}

; A denormal threshold: with x at zero, x >= 1e-40 is true when flushed and false
; when retained, so none of these may fold.
; CHECK-LABEL: define void @denormal_literal_threshold(
; CHECK: fcmp nnan oge
; CHECK: fcmp nnan olt
; CHECK: fcmp nnan oeq
define void @denormal_literal_threshold(float %v) {
  %hi = call float @llvm.minnum.f32(float %v, float 5.000000e+00)
  %x  = call float @llvm.maxnum.f32(float %hi, float 0x37A16C2000000000)
  %c1 = fcmp nnan oge float %x, 0x37A16C2000000000
  %c2 = fcmp nnan olt float %x, 0x37A16C2000000000
  %c3 = fcmp nnan oeq float %x, 0x37A16C2000000000
  call void @use(i1 %c1)
  call void @use(i1 %c2)
  call void @use(i1 %c3)
  ret void
}
