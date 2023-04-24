;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -mattr=+emulate_i64 -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_sub
; CHECK-NEXT: %arg1.split0 = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %arg1, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %arg2.split0 = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %arg2, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %res.split0 = sub <8 x i64> %arg1.split0, %arg2.split0
; CHECK-NEXT: %res.split0.join0 = call <16 x i64> @llvm.genx.wrregioni.v16i64.v8i64.i16.i1(<16 x i64> undef, <8 x i64> %res.split0, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: %arg1.split8 = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %arg1, i32 8, i32 8, i32 1, i16 64, i32 undef)
; CHECK-NEXT: %arg2.split8 = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %arg2, i32 8, i32 8, i32 1, i16 64, i32 undef)
; CHECK-NEXT: %res.split8 = sub <8 x i64> %arg1.split8, %arg2.split8
; CHECK-NEXT: %res.split8.join8 = call <16 x i64> @llvm.genx.wrregioni.v16i64.v8i64.i16.i1(<16 x i64> %res.split0.join0, <8 x i64> %res.split8, i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
define <16 x i64> @test_sub(<16 x i64> %arg1, <16 x i64> %arg2) {
  %res = sub <16 x i64> %arg1, %arg2
  ret <16 x i64> %res
}
