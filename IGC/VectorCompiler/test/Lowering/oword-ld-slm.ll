;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <2 x i64> @llvm.genx.oword.ld.v2i64(i32, i32, i32)
declare <16 x i64> @llvm.genx.oword.ld.v16i64(i32, i32, i32)

define <2 x i64> @test1.v2i64(i32 %arg) {
  %shl = shl i32 %arg, 3
  %and = and i32 %shl, 268435448
  %ret = tail call <2 x i64> @llvm.genx.oword.ld.v2i64(i32 0, i32 254, i32 %and)
  ret <2 x i64> %ret
; CHECK-LABEL: define <2 x i64> @test1.v2i64(i32 %arg)
; CHECK: [[SHL:%.*]] = shl i32 %arg, 3
; CHECK-NEXT: [[AND:%.*]] = and i32 [[SHL]], 268435448
; CHECK-NEXT: [[RET:%.*]] = tail call <2 x i64> @llvm.genx.oword.ld.v2i64(i32 0, i32 254, i32 [[AND]])
; CHECK-NEXT: ret <2 x i64> [[RET]]
}

define <16 x i64> @test8.v16i64(i32 %arg) {
  %shl = shl i32 %arg, 3
  %and = and i32 %shl, 268435448
  %ret = tail call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 254, i32 %and)
  ret <16 x i64> %ret
; CHECK-LABEL: define <16 x i64> @test8.v16i64(i32 %arg)
; CHECK: [[SHL:%.*]] = shl i32 %arg, 3
; CHECK-NEXT: [[AND:%.*]] = and i32 [[SHL]], 268435448
; CHECK-NEXT: [[RET:%.*]] = tail call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 254, i32 [[AND]])
; CHECK-NEXT: ret <16 x i64> [[RET]]
}
