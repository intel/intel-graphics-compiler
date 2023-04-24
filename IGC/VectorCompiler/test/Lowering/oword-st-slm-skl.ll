;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare void @llvm.genx.oword.st.v2i64(i32, i32, <2 x i64>)
declare void @llvm.genx.oword.st.v16i64(i32, i32, <16 x i64>)

define void @test1.v2i64(i32 %off, <2 x i64> %arg) {
  call void @llvm.genx.oword.st.v2i64(i32 254, i32 %off, <2 x i64> %arg)
; CHECK: [[SHL:%.*]] = shl i32 %off, 4
; CHECK-NEXT: [[CAST:%.*]] = bitcast <2 x i64> %arg to <4 x i32>
; CHECK-NEXT: call void @llvm.genx.scatter.scaled.v4i1.v4i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[SHL]], <4 x i32> <i32 0, i32 4, i32 8, i32 12>, <4 x i32> [[CAST]])
  ret void
}

define void @test8.v16i64(i32 %off, <16 x i64> %arg) {
  call void @llvm.genx.oword.st.v16i64(i32 254, i32 %off, <16 x i64> %arg)
; CHECK: [[SHL:%.*]] = shl i32 %off, 4
; CHECK-NEXT: [[CAST:%.*]] = bitcast <16 x i64> %arg to <32 x i32>
; CHECK-NEXT: [[RD1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[SHL]], <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>, <16 x i32> [[RD1]])
; CHECK-NEXT: [[ADD:%.*]] = add i32 [[SHL]], 64
; CHECK-NEXT: [[RD2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 2, i16 0, i32 254, i32 [[ADD]], <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>, <16 x i32> [[RD2]])
  ret void
}

