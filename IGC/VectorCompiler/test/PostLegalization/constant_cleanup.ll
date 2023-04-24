;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

;; Test cleanup of genx.constant with non-constant operand

declare <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32>)
declare <8 x i64> @llvm.genx.constanti.v8i64(<8 x i64>)

define <8 x i64> @test() {
  %constant.1 = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
  %constantscale = mul <8 x i32> %constant.1, <i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4>
  %constantsext = sext <8 x i32> %constantscale to <8 x i64>
  %constant = call <8 x i64> @llvm.genx.constanti.v8i64(<8 x i64> %constantsext)
; CHECK: ret <8 x i64> %constantsext
  ret <8 x i64> %constant
}
