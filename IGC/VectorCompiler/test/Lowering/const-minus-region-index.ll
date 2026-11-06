;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Several "sub C, X" used as region indices of the same X in one basic block
; are rewritten to "add Base, C - Cmin" with a single shared
; "Base = sub Cmin, X", where Cmin is the smallest constant among the subs that
; are directly a region index. This lets GenXCategory turn the constants into
; llvm.genx.add.addr offsets of one address base. The base is itself a valid
; (non-negative) index, as GenXAddressCommoning requires, because it is equal
; to one of the original indices.

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @const_minus_index(
define <4 x i32> @const_minus_index(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
  %a = sub i16 6, %shl
  %b = sub i16 8, %shl
  %c = sub i16 10, %shl
  %c.or = or i16 %c, 1
; CHECK: %shl = shl i16 %x, 1
; CHECK-NEXT: %shl.cmbase = sub i16 6, %shl
; CHECK: [[B:%[^ ]+]] = add i16 %shl.cmbase, 2
; CHECK: [[C:%[^ ]+]] = add i16 %shl.cmbase, 4
; CHECK: [[COR:%[^ ]+]] = or i16 [[C]], 1
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %shl.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[COR]], i32 undef)
; CHECK-NOT: sub i16 {{8|10}}, %shl
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %r2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %c.or, i32 undef)
  %s0 = add <4 x i32> %r0, %r1
  %s1 = add <4 x i32> %s0, %r2
  ret <4 x i32> %s1
}

; Cmin is the smallest constant, not the first one in program order.
; CHECK-LABEL: @smallest_constant_is_base(
define <4 x i32> @smallest_constant_is_base(<64 x i32> %src, i16 %x) {
  %m = mul i16 %x, 3
  %a = sub i16 12, %m
  %b = sub i16 4, %m
; CHECK: %m.cmbase = sub i16 4, %m
; CHECK: [[A:%[^ ]+]] = add i16 %m.cmbase, 8
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[A]], i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %m.cmbase, i32 undef)
; CHECK-NOT: sub i16 12, %m
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %r0, %r1
  ret <4 x i32> %s
}

; X is a function argument: the base is created in front of the first index.
; CHECK-LABEL: @arg_base(
define <4 x i32> @arg_base(<64 x i32> %src, i16 %x) {
  %a = sub i16 6, %x
  %b = sub i16 9, %x
; CHECK: %x.cmbase = sub i16 6, %x
; CHECK: [[B:%[^ ]+]] = add i16 %x.cmbase, 3
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-NOT: sub i16 9, %x
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %r0, %r1
  ret <4 x i32> %s
}

; X is an instruction: the base is created in front of the first index.
; CHECK-LABEL: @inst_base(
define <4 x i32> @inst_base(<64 x i32> %src, i16 %x) {
  %v = xor i16 %x, 5
  %a = sub i16 6, %v
  %b = sub i16 7, %v
; CHECK: %v = xor i16 %x, 5
; CHECK-NEXT: %v.cmbase = sub i16 6, %v
; CHECK: [[B:%[^ ]+]] = add i16 %v.cmbase, 1
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %v.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-NOT: sub i16 7, %v
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %r0, %r1
  ret <4 x i32> %s
}

; X is a PHI: the base is created in front of the first index, after the PHIs.
; CHECK-LABEL: @phi_base(
define <4 x i32> @phi_base(<64 x i32> %src, i16 %x, i1 %c) {
entry:
  br label %loop
loop:
; CHECK: %p = phi i16
  %p = phi i16 [ %x, %entry ], [ %p.next, %loop ]
  %p.next = add i16 %p, 1
  %a = sub i16 4, %p
  %b = sub i16 6, %p
; CHECK: %p.cmbase = sub i16 4, %p
; CHECK: [[B:%[^ ]+]] = add i16 %p.cmbase, 2
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %p.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-NOT: sub i16 6, %p
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %r0, %r1
  br i1 %c, label %loop, label %exit
exit:
  ret <4 x i32> %s
}

; wrregion index users are handled too.
; CHECK-LABEL: @wrregion_index(
define <64 x i32> @wrregion_index(<64 x i32> %old, <4 x i32> %new, i16 %x) {
  %shl = shl i16 %x, 2
  %a = sub i16 8, %shl
  %b = sub i16 10, %shl
; CHECK: %shl.cmbase = sub i16 8, %shl
; CHECK: [[B:%[^ ]+]] = add i16 %shl.cmbase, 2
; CHECK: [[W0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> %old, <4 x i32> %new, i32 0, i32 4, i32 1, i16 %shl.cmbase, i32 undef, i1 true)
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> [[W0]], <4 x i32> %new, i32 0, i32 4, i32 1, i16 [[B]], i32 undef, i1 true)
; CHECK-NOT: sub i16 10, %shl
  %w0 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> %old, <4 x i32> %new, i32 0, i32 4, i32 1, i16 %a, i32 undef, i1 true)
  %w1 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v4i32.i16.i1(<64 x i32> %w0, <4 x i32> %new, i32 0, i32 4, i32 1, i16 %b, i32 undef, i1 true)
  ret <64 x i32> %w1
}

; The indices are only shared inside a basic block: Cmin is taken from the
; indices of the same block, so the base is never computed on a path where the
; index with the smallest constant is not executed.
; CHECK-LABEL: @cross_block(
define <4 x i32> @cross_block(<64 x i32> %src, i16 %x, i1 %c) {
entry:
  br i1 %c, label %t, label %f
t:
; CHECK: %a = sub i16 4, %x
  %a = sub i16 4, %x
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  br label %exit
f:
; CHECK: %x.cmbase = sub i16 12, %x
; CHECK: [[B:%[^ ]+]] = add i16 %x.cmbase, 2
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-NOT: sub i16 14, %x
  %b = sub i16 12, %x
  %b2 = sub i16 14, %x
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %rb2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b2, i32 undef)
  %rbs = add <4 x i32> %rb, %rb2
  br label %exit
exit:
  %r = phi <4 x i32> [ %ra, %t ], [ %rbs, %f ]
  ret <4 x i32> %r
}

; Cmin only comes from subs that are directly a region index. A sub reached
; through an add chain is not known to be a valid index on its own (the chain
; constant brings the final index in range), so it must not become the base
; even if its constant is the smallest. It is left alone here.
; CHECK-LABEL: @chain_only_smaller_constant(
define <4 x i32> @chain_only_smaller_constant(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 4, %x
; CHECK: %x.cmbase = sub i16 12, %x
; CHECK: [[C:%[^ ]+]] = add i16 %x.cmbase, 2
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[C]], i32 undef)
; CHECK-NOT: sub i16 14, %x
  %a = sub i16 4, %x
  %a.add = add i16 %a, 8
  %b = sub i16 12, %x
  %c = sub i16 14, %x
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a.add, i32 undef)
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %rc = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %c, i32 undef)
  %s0 = add <4 x i32> %ra, %rb
  %s1 = add <4 x i32> %s0, %rc
  ret <4 x i32> %s1
}

; Without any direct index there is no known valid index to use as a base.
; CHECK-LABEL: @chain_only(
define <4 x i32> @chain_only(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 6, %x
; CHECK: %b = sub i16 8, %x
; CHECK-NOT: cmbase
  %a = sub i16 6, %x
  %a.or = or i16 %a, 1
  %b = sub i16 8, %x
  %b.or = or i16 %b, 1
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a.or, i32 undef)
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b.or, i32 undef)
  %s = add <4 x i32> %ra, %rb
  ret <4 x i32> %s
}

; A sub that is a direct region index and also feeds an "or" chain is a valid
; base candidate.
; CHECK-LABEL: @direct_and_or(
define <4 x i32> @direct_and_or(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
  %a = sub i16 6, %shl
  %a.or = or i16 %a, 1
  %b = sub i16 8, %shl
  %b.or = or i16 %b, 1
; CHECK: %shl.cmbase = sub i16 6, %shl
; CHECK: %a.or = or i16 %shl.cmbase, 1
; CHECK: [[B:%[^ ]+]] = add i16 %shl.cmbase, 2
; CHECK: %b.or = or i16 [[B]], 1
; CHECK-NOT: sub i16 8, %shl
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a.or, i32 undef)
  %r2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %r3 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b.or, i32 undef)
  %s0 = add <4 x i32> %r0, %r1
  %s1 = add <4 x i32> %s0, %r2
  %s2 = add <4 x i32> %s1, %r3
  ret <4 x i32> %s2
}

; A sub that only reaches the region through an "or" chain is rewritten when
; its constant is not below Cmin, which comes from the direct index.
; CHECK-LABEL: @chain_only_above_cmin(
define <4 x i32> @chain_only_above_cmin(<64 x i32> %src, i16 %x) {
  %a = sub i16 4, %x
  %b = sub i16 10, %x
  %b.or = or i16 %b, 1
; CHECK: %x.cmbase = sub i16 4, %x
; CHECK: [[B:%[^ ]+]] = add i16 %x.cmbase, 6
; CHECK: %b.or = or i16 [[B]], 1
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b.or, i32 undef)
; CHECK-NOT: sub i16 10, %x
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b.or, i32 undef)
  %s = add <4 x i32> %ra, %rb
  ret <4 x i32> %s
}

; Cmin only comes from subs whose direct region index use is in the same block:
; a use in a conditional successor does not make the sub a valid index on the
; paths that skip that block. The sub 4 is used only in %t, so it must not
; become the base for the indices used in %entry; it is left alone.
; CHECK-LABEL: @use_in_other_block(
define <4 x i32> @use_in_other_block(<64 x i32> %src, i16 %x, i1 %c) {
entry:
; CHECK: %a = sub i16 4, %x
; CHECK: %x.cmbase = sub i16 12, %x
; CHECK: [[C:%[^ ]+]] = add i16 %x.cmbase, 2
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[C]], i32 undef)
; CHECK-NOT: sub i16 14, %x
  %a = sub i16 4, %x
  %b = sub i16 12, %x
  %c.idx = sub i16 14, %x
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %rc = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %c.idx, i32 undef)
  %s = add <4 x i32> %rb, %rc
  br i1 %c, label %t, label %exit
t:
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %s.t = add <4 x i32> %s, %ra
  br label %exit
exit:
  %r = phi <4 x i32> [ %s, %entry ], [ %s.t, %t ]
  ret <4 x i32> %r
}

; The indices may be used in another block, as long as the use of the sub with
; the smallest constant dominates all the others: that index has then already
; been executed with the same X.
; CHECK-LABEL: @uses_in_dominated_block(
define <4 x i32> @uses_in_dominated_block(<64 x i32> %src, i16 %x) {
entry:
  %a = sub i16 4, %x
  %b = sub i16 6, %x
  br label %body
body:
; CHECK: %x.cmbase = sub i16 4, %x
; CHECK: [[B:%[^ ]+]] = add i16 %x.cmbase, 2
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %x.cmbase, i32 undef)
; CHECK-DAG: call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 [[B]], i32 undef)
; CHECK-NOT: sub i16 6, %x
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %ra, %rb
  ret <4 x i32> %s
}

; The indices are used in different conditional blocks: neither use dominates
; the other, so the index with the smallest constant is not known to be valid
; where the other one is used. Nothing is rewritten.
; CHECK-LABEL: @conditional_uses(
define <4 x i32> @conditional_uses(<64 x i32> %src, i16 %x, i1 %c) {
entry:
; CHECK: %a = sub i16 4, %x
; CHECK: %b = sub i16 12, %x
; CHECK-NOT: cmbase
  %a = sub i16 4, %x
  %b = sub i16 12, %x
  br i1 %c, label %t, label %f
t:
  %ra = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  br label %exit
f:
  %rb = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  br label %exit
exit:
  %r = phi <4 x i32> [ %ra, %t ], [ %rb, %f ]
  ret <4 x i32> %r
}

; A lone index has nothing to share a base with: it is left alone.
; CHECK-LABEL: @lone_index(
define <4 x i32> @lone_index(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
; CHECK: %a = sub i16 6, %shl
; CHECK-NOT: cmbase
  %a = sub i16 6, %shl
  %r = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  ret <4 x i32> %r
}

; Subs with a non-index user are left alone and do not count as indices, so
; the remaining single index is a lone one as well.
; CHECK-LABEL: @const_minus_other_use(
define i16 @const_minus_other_use(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 6, %x
; CHECK: %b = sub i16 8, %x
; CHECK-NOT: cmbase
  %a = sub i16 6, %x
  %b = sub i16 8, %x
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %e = extractelement <4 x i32> %r0, i32 0
  %t = trunc i32 %e to i16
  %res = add i16 %t, %a
  %e1 = extractelement <4 x i32> %r1, i32 0
  %t1 = trunc i32 %e1 to i16
  %res1 = add i16 %res, %t1
  ret i16 %res1
}

; A non-index user further down the add/sub/or chain also blocks the rewrite.
; CHECK-LABEL: @const_minus_chain_other_use(
define i16 @const_minus_chain_other_use(<64 x i32> %src, i16 %x) {
  %shl = shl i16 %x, 1
; CHECK: %a = sub i16 6, %shl
; CHECK: %b = sub i16 8, %shl
; CHECK-NOT: cmbase
  %a = sub i16 6, %shl
  %b = sub i16 8, %shl
  %o = or i16 %a, 1
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %o, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %e = extractelement <4 x i32> %r0, i32 0
  %t = trunc i32 %e to i16
  %res = add i16 %t, %o
  %e1 = extractelement <4 x i32> %r1, i32 0
  %t1 = trunc i32 %e1 to i16
  %res1 = add i16 %res, %t1
  ret i16 %res1
}

; "sub 0, X" is already a plain negation and is left alone.
; CHECK-LABEL: @zero_minus_index(
define <4 x i32> @zero_minus_index(<64 x i32> %src, i16 %x) {
; CHECK: %a = sub i16 0, %x
; CHECK-NOT: cmbase
  %a = sub i16 0, %x
  %b = sub i16 0, %x
  %r0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %a, i32 undef)
  %r1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v64i32.i16(<64 x i32> %src, i32 0, i32 4, i32 1, i16 %b, i32 undef)
  %s = add <4 x i32> %r0, %r1
  ret <4 x i32> %s
}
