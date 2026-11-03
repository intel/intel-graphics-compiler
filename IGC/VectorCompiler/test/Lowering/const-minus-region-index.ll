;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; "sub C, X" used as a region index must be rewritten to "add (neg X), C" with
; a single shared neg(X), so that GenXCategory can turn the constant into an
; llvm.genx.add.addr offset instead of building an address per index.

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

; X is "shl Y, k": neg(X) becomes "mul Y, -(1 << k)", shared by all the subs.
; CHECK-LABEL: @const_minus_index(
define <4 x i32> @const_minus_index(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
  %a = sub i16 6, %shl
  %b = sub i16 8, %shl
  %c = sub i16 10, %shl
  %c.or = or i16 %c, 1
; CHECK: %shl.neg = mul i16 %x, -2
; CHECK: [[A:%[^ ]+]] = add i16 %shl.neg, 6
; CHECK: [[B:%[^ ]+]] = add i16 %shl.neg, 8
; CHECK: [[C:%[^ ]+]] = add i16 %shl.neg, 10
; CHECK: [[COR:%[^ ]+]] = or i16 [[C]], 1
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[COR]], i32 undef)
; CHECK-NOT: sub i16
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %r2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %c.or, i32 undef)
  %s0 = add <4 x i32> %r0, %r1
  %s1 = add <4 x i32> %s0, %r2
  ret <4 x i32> %s1
}

; X is "mul Y, 3": neg(X) becomes "mul Y, -3".
; CHECK-LABEL: @mul_base(
define <4 x i32> @mul_base(<64 x i32> %src, i16 %x) {
  %m = mul i16 %x, 3
  %a = sub i16 10, %m
; CHECK: %m.neg = mul i16 %x, -3
; CHECK: [[A:%[^ ]+]] = add i16 %m.neg, 10
; CHECK: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-NOT: sub i16
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}

; Negating the minimum signed constant must wrap instead of overflowing.
; CHECK-LABEL: @mul_min_signed(
define <4 x i32> @mul_min_signed(<64 x i32> %src, i16 %x) {
  %m = mul i16 %x, -32768
  %a = sub i16 10, %m
; CHECK: %m.neg = mul i16 %x, -32768
; CHECK: [[A:%[^ ]+]] = add i16 %m.neg, 10
; CHECK: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-NOT: sub i16 10
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}

; X is a function argument: plain neg, created at the start of the entry block.
; CHECK-LABEL: @arg_neg(
define <4 x i32> @arg_neg(<64 x i32> %src, i16 %x) {
  %a = sub i16 6, %x
; CHECK: %x.neg = sub i16 0, %x
; CHECK: [[A:%[^ ]+]] = add i16 %x.neg, 6
; CHECK: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-NOT: sub i16 6
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}

; X is an instruction that is neither shl nor mul: plain neg right after it.
; CHECK-LABEL: @inst_neg(
define <4 x i32> @inst_neg(<64 x i32> %src, i16 %x) {
  %v = xor i16 %x, 5
  %a = sub i16 6, %v
; CHECK: %v = xor i16 %x, 5
; CHECK-NEXT: %v.neg = sub i16 0, %v
; CHECK: [[A:%[^ ]+]] = add i16 %v.neg, 6
; CHECK: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-NOT: sub i16 6
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}

; X is a PHI: the neg is created after the PHIs of its block.
; CHECK-LABEL: @phi_neg(
define <4 x i32> @phi_neg(<64 x i32> %src, i16 %x, i1 %c) {
entry:
  br label %loop
loop:
; CHECK: %p = phi i16
; CHECK-NEXT: %p.neg = sub i16 0, %p
  %p = phi i16 [ %x, %entry ], [ %p.next, %loop ]
  %p.next = add i16 %p, 1
  %a = sub i16 4, %p
; CHECK: [[A:%[^ ]+]] = add i16 %p.neg, 4
; CHECK: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-NOT: sub i16 4
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  br i1 %c, label %loop, label %exit
exit:
  ret <4 x i32> %r
}

; wrregion index users are handled too.
; CHECK-LABEL: @wrregion_index(
define <64 x i32> @wrregion_index(<64 x i32> %old, <4 x i32> %new, i16 %x) {
  %shl = shl i16 %x, 2
  %a = sub i16 8, %shl
; CHECK: %shl.neg = mul i16 %x, -4
; CHECK: [[A:%[^ ]+]] = add i16 %shl.neg, 8
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> %old, <4 x i32> %new, i32 0, i32 4, i32 1, i16 [[A]], i32 undef, i1 true)
; CHECK-NOT: sub i16 8
  %w = call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> %old, <4 x i32> %new, i32 0, i32 4, i32 1, i16 %a, i32 undef, i1 true)
  ret <64 x i32> %w
}

; The sub has a non-index user, so it is left alone.
; CHECK-LABEL: @const_minus_other_use(
define i16 @const_minus_other_use(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 6, %x
; CHECK-NOT: .neg
  %a = sub i16 6, %x
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %e = extractelement <4 x i32> %r0, i32 0
  %t = trunc i32 %e to i16
  %res = add i16 %t, %a
  ret i16 %res
}

; A non-index user further down the add/sub/or chain also blocks the rewrite.
; CHECK-LABEL: @const_minus_chain_other_use(
define i16 @const_minus_chain_other_use(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
; CHECK: %a = sub i16 6, %shl
; CHECK-NOT: .neg
  %a = sub i16 6, %shl
  %o = or i16 %a, 1
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %o, i32 undef)
  %e = extractelement <4 x i32> %r0, i32 0
  %t = trunc i32 %e to i16
  %res = add i16 %t, %o
  ret i16 %res
}

; "sub 0, X" is already a plain negation and is left alone.
; CHECK-LABEL: @zero_minus_index(
define <4 x i32> @zero_minus_index(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 0, %x
; CHECK-NOT: .neg
  %a = sub i16 0, %x
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}
