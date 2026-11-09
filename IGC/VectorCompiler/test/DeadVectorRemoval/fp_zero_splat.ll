;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_23_or_greater
; RUN: %opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: LLVM 23 represents a zero FP splat as a vector ConstantFP. When some of
; COM: its lanes are dead, it must stay a splat so that it is emitted as a
; COM: scalar immediate, not as a packed vector immediate.

declare <1 x i16> @llvm.genx.rdregioni.v1i16.v2i16.i16(<2 x i16>, i32, i32, i32, i16, i32)

define <1 x i16> @test_fp_zero_splat(<2 x float> %x) {
; CHECK-LABEL: @test_fp_zero_splat(
; CHECK: fcmp oeq <2 x float> %x, zeroinitializer
  %c = fcmp oeq <2 x float> %x, splat (float 0.0)
  %s = select <2 x i1> %c, <2 x i16> splat (i16 1), <2 x i16> zeroinitializer
  %r = call <1 x i16> @llvm.genx.rdregioni.v1i16.v2i16.i16(<2 x i16> %s, i32 0, i32 1, i32 0, i16 0, i32 undef)
  ret <1 x i16> %r
}
