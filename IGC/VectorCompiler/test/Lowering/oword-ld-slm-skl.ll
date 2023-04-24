;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <2 x i64> @llvm.genx.oword.ld.v2i64(i32, i32, i32)
declare <16 x i64> @llvm.genx.oword.ld.v16i64(i32, i32, i32)

define <2 x i64> @test1.v2i64(i32 %arg) {
  %shl = shl i32 %arg, 3
  %and = and i32 %shl, 268435448
  %ret = tail call <2 x i64> @llvm.genx.oword.ld.v2i64(i32 0, i32 254, i32 %and)
  ret <2 x i64> %ret
; CHECK: [[SHL:%.*]] = shl i32 %and, 4
; CHECK-NEXT: [[GAT:%.*]] = call <4 x i32> @llvm.genx.gather.scaled.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[SHL]], <4 x i32> <i32 0, i32 4, i32 8, i32 12>, <4 x i32> undef)
; CHECK-NEXT: [[CAST:%.*]] = bitcast <4 x i32> [[GAT]] to <2 x i64>
; CHECK-NEXT: ret <2 x i64> [[CAST]]
}

define <16 x i64> @test8.v16i64(i32 %arg) {
  %shl = shl i32 %arg, 3
  %and = and i32 %shl, 268435448
  %ret = tail call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 254, i32 %and)
  ret <16 x i64> %ret
; CHECK: [[SHL:%.*]] = shl i32 %and, 4
; CHECK-NEXT: [[GAT1:%.*]] = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[SHL]], <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>, <16 x i32> undef)
; CHECK-NEXT: [[ADD:%.*]] = add i32 [[SHL]], 64
; CHECK-NEXT: [[GAT2:%.*]] = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[ADD]], <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>, <16 x i32> undef)
; CHECK-NEXT: [[WR1:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[GAT1]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[WR2:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[WR1]], <16 x i32> [[GAT2]], i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK-NEXT: [[CAST:%.*]] = bitcast <32 x i32> [[WR2]] to <16 x i64>
; CHECK-NEXT: ret <16 x i64> [[CAST]]
}
