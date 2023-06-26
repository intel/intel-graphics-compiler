;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <8 x i32> @llvm.genx.wrregioni.v8i32.v2i32.i16.i1(<8 x i32>, <2 x i32>, i32, i32, i32, i16, i32, i1) readnone nounwind

define <4 x i64> @test(i64 %val) {
; CHECK: %1 = bitcast <4 x i64> <i64 undef, i64 2, i64 3, i64 4> to <8 x i32>
  %1 = bitcast <4 x i64> <i64 1, i64 2, i64 3, i64 4> to <8 x i32>
  %2 = bitcast i64 %val to <2 x i32>
  %3 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v2i32.i16.i1(<8 x i32> %1, <2 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true);
  %4 = bitcast <8 x i32> %3 to <4 x i64>
  ret <4 x i64> %4
}
