;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; LLVM's reassociate pass rewrites a load address "(base + index) + const" into
; "(base + const) + index". The outer add then has no constant operand, which
; previously prevented GenXLscAddrCalcFolding from folding the constant into the
; message immediate offset and from exposing the shared "base + index" base for
; CSE. Verify the pass reassociates "(X +/- C) + Y" back to "(X + Y) +/- C".

declare <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <2 x i64>)

; Constant on the RHS of the inner add: (base + 32) + index -> (base + index), off 32
; CHECK-LABEL: test_reassoc_add_rhs
define <2 x i64> @test_reassoc_add_rhs(i64 %base, i64 %index) {
; CHECK: %[[BASE:[a-zA-Z0-9._]+]] = add i64 %base, %index
; CHECK: tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %[[BASE]], i16 1, i32 32, <2 x i64> undef)
  %inner = add i64 %base, 32
  %addr = add i64 %inner, %index
  %data = tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  ret <2 x i64> %data
}

; Constant on the LHS of the inner add: (32 + base) + index -> (base + index), off 32
; CHECK-LABEL: test_reassoc_add_lhs
define <2 x i64> @test_reassoc_add_lhs(i64 %base, i64 %index) {
; CHECK: %[[BASE:[a-zA-Z0-9._]+]] = add i64 %base, %index
; CHECK: tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %[[BASE]], i16 1, i32 32, <2 x i64> undef)
  %inner = add i64 32, %base
  %addr = add i64 %inner, %index
  %data = tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  ret <2 x i64> %data
}

; Inner subtraction: (base - 32) + index -> (base + index), off -32
; CHECK-LABEL: test_reassoc_sub
define <2 x i64> @test_reassoc_sub(i64 %base, i64 %index) {
; CHECK: %[[BASE:[a-zA-Z0-9._]+]] = add i64 %base, %index
; CHECK: tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %[[BASE]], i16 1, i32 -32, <2 x i64> undef)
  %inner = sub i64 %base, 32
  %addr = add i64 %inner, %index
  %data = tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  ret <2 x i64> %data
}

; The inner add has more than one use, so reassociating it would not reduce the
; instruction count. The address must be left unchanged.
; CHECK-LABEL: test_no_reassoc_multiuse
define <2 x i64> @test_no_reassoc_multiuse(i64 %base, i64 %index, i64* %p) {
; CHECK: %inner = add i64 %base, 32
; CHECK: %addr = add i64 %inner, %index
; CHECK: tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  %inner = add i64 %base, 32
  %addr = add i64 %inner, %index
  store i64 %inner, i64* %p
  %data = tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  ret <2 x i64> %data
}

; "C - X" cannot be reassociated into the immediate offset; leave it unchanged.
; CHECK-LABEL: test_no_reassoc_const_minus_x
define <2 x i64> @test_no_reassoc_const_minus_x(i64 %base, i64 %index) {
; CHECK: %inner = sub i64 32, %base
; CHECK: %addr = add i64 %inner, %index
; CHECK: tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  %inner = sub i64 32, %base
  %addr = add i64 %inner, %index
  %data = tail call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.i1.v2i8.i64(i1 true, i8 3, i8 4, i8 2, <2 x i8> <i8 2, i8 2>, i64 0, i64 %addr, i16 1, i32 0, <2 x i64> undef)
  ret <2 x i64> %data
}
