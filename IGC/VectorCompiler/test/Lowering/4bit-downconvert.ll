;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32>, <16 x i32>, <16 x i32>, i8, i8, i8)

; CHECK-LABEL: @test_srnd(
define <16 x i32> @test_srnd(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c) {
; CHECK: %res = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c, i8 0, i8 0, i8 0)
  %res = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c, i8 0, i8 0, i8 0)
  ret <16 x i32> %res
}

; CHECK-LABEL: @test_rne(
define <16 x i32> @test_rne(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c) {
; CHECK: %res = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> undef, i8 0, i8 0, i8 1)
  %res = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c, i8 0, i8 0, i8 1)
  ret <16 x i32> %res
}
