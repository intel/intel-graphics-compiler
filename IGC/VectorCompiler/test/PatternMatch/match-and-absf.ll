;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test(
define <4 x float> @test(<4 x float> %a, <4 x float> %b) {
; CHECK: [[ABS:%[^ ]+]] = call <4 x float> @llvm.genx.absf.v4f32(<4 x float> %a)
; CHECK: [[ADD:%[^ ]+]] = fadd <4 x float> [[ABS]], %b
  %cast = bitcast <4 x float> %a to <4 x i32>
  %abs = and <4 x i32> %cast, <i32 2147483647, i32 2147483647, i32 2147483647, i32 2147483647>
  %fcast = bitcast <4 x i32> %abs to <4 x float>
  %c = fadd <4 x float> %fcast, %b
  ret <4 x float> %c
}
