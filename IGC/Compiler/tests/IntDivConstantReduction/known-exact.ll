;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-intdiv-red -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction: a signed div/rem by +-2^k whose dividend is a known
; multiple of 2^k is expanded without the rounding fix-up.
; ------------------------------------------------

declare void @llvm.assume(i1)
declare void @use.i32(i32)

; (gid * 8) / 2, (gid * 8) % 2, (gid * 8) / -4: exact, no is-neg.
define spir_kernel void @exact_pow2(i32 %gid) {
; CHECK-LABEL: @exact_pow2(
; CHECK-NOT:     is-neg
; CHECK:         [[B:%.*]] = shl i32 %gid, 3
; CHECK-NEXT:    [[Q:%.*]] = ashr exact i32 [[B]], 1
; CHECK-NEXT:    [[Q4:%.*]] = ashr exact i32 [[B]], 2
; CHECK-NEXT:    [[N4:%.*]] = sub i32 0, [[Q4]]
; CHECK-NEXT:    call void @use.i32(i32 [[Q]])
; CHECK-NEXT:    call void @use.i32(i32 0)
; CHECK-NEXT:    call void @use.i32(i32 [[N4]])
; CHECK-NEXT:    ret void
  %base = shl i32 %gid, 3
  %q = sdiv i32 %base, 2
  %r = srem i32 %base, 2
  %qn = sdiv i32 %base, -4
  call void @use.i32(i32 %q)
  call void @use.i32(i32 %r)
  call void @use.i32(i32 %qn)
  ret void
}

; sdiv exact needs no trailing-zero proof; INT_MIN divisor with 31 known zeros.
define spir_kernel void @exact_flag_and_minval(i32 %x, i32 %y) {
; CHECK-LABEL: @exact_flag_and_minval(
; CHECK-NOT:     is-neg
; CHECK:         [[Q:%.*]] = ashr exact i32 %x, 3
; CHECK:         [[S:%.*]] = shl i32 %y, 31
; CHECK-NEXT:    [[A:%.*]] = ashr exact i32 [[S]], 31
; CHECK-NEXT:    [[N:%.*]] = sub i32 0, [[A]]
; CHECK-NEXT:    call void @use.i32(i32 [[Q]])
; CHECK-NEXT:    call void @use.i32(i32 [[N]])
  %q = sdiv exact i32 %x, 8
  %s = shl i32 %y, 31
  %m = sdiv i32 %s, -2147483648
  call void @use.i32(i32 %q)
  call void @use.i32(i32 %m)
  ret void
}

; Too few trailing zeros (1 < log2(4)): the signed expansion stays.
define spir_kernel void @not_exact(i32 %x) {
; CHECK-LABEL: @not_exact(
; CHECK:         [[B:%.*]] = shl i32 %x, 1
; CHECK-NEXT:    [[ISNEG:%.*]] = icmp slt i32 [[B]], 0
; CHECK:         add i32 [[B]], 3
; CHECK:         ashr i32 {{.*}}, 2
  %b = shl i32 %x, 1
  %q = sdiv i32 %b, 4
  call void @use.i32(i32 %q)
  ret void
}

; A dividend that is only known non-negative keeps the signed expansion here:
; InstCombine already turns such an sdiv into udiv before this pass runs.
define spir_kernel void @non_negative_not_handled(i32 %lid) {
; CHECK-LABEL: @non_negative_not_handled(
; CHECK:         icmp slt i32 %lid, 0
; CHECK:         ashr i32 {{.*}}, 1
  %in = icmp ult i32 %lid, 64
  call void @llvm.assume(i1 %in)
  %pos = sdiv i32 %lid, 2
  call void @use.i32(i32 %pos)
  ret void
}
