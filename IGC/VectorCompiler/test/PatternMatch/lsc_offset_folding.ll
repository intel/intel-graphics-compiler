;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1>, i8, i8, i8, <2 x i8>, i32, <4 x i32>, i16, i32, <4 x i64>)

; CHECK-LABEL: test1
define <4 x i64> @test1(<4 x i32> %arg) {
; CHECK: %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %arg, i16 1, i32 16, <4 x i64> undef)
  %addr = add <4 x i32> %arg, <i32 16, i32 16, i32 16, i32 16>
  %data = tail call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 0, <4 x i32> %addr, i16 1, i32 0, <4 x i64> undef)
  ret <4 x i64> %data
}
