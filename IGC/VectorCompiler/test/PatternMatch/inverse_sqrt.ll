;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_inverse
define <16 x float> @test_inverse(<16 x float> %val) {
  %sqrt = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %val)
; CHECK: [[INVERSE_SQRT:%.*]] = call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
; CHECK-NEXT: ret <16 x float> [[INVERSE_SQRT]]
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  ret <16 x float> %inv
}

; CHECK-LABEL: @test_inverse2
define <16 x float> @test_inverse2(<16 x float> %val1, <16 x float> %val2) {
; CHECK: [[INVERSE_SQRT21:%.*]] = call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val1)
; CHECK-NEXT: [[INVERSE_SQRT22:%.*]] = call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val2)
; CHECK-NEXT: [[FADD:%.*]] = fadd <16 x float>  [[INVERSE_SQRT21]], [[INVERSE_SQRT22]]
; CHECK-NEXT: ret <16 x float> [[FADD]]
  %sqrt1 = call fast <16 x float> @llvm.sqrt.v16f32(<16 x float> %val1)
  %inv1 = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt1)
  %sqrt2 = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %val2)
  %inv2 = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt2)
  %add = fadd <16 x float>  %inv1, %inv2
  ret <16 x float> %add
}
; CHECK-LABEL: @test_inverse_scalar
define float @test_inverse_scalar(float %val) {
  %sqrt = call float @llvm.genx.sqrt.f32(float %val)
; CHECK: [[INVERSE_SQRT_SCALAR:%.*]] = call float @llvm.genx.rsqrt.f32(float %val)
; CHECK-NEXT: ret float [[INVERSE_SQRT_SCALAR]]
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_inverse_fast
define <16 x float> @test_inverse_fast(<16 x float> %val) {
  %sqrt = call fast <16 x float> @llvm.sqrt.v16f32(<16 x float> %val)
; CHECK: [[INVERSE_SQRT_FAST:%.*]] = call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
; CHECK-NEXT: ret <16 x float> [[INVERSE_SQRT_FAST]]
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  ret <16 x float> %inv
}

; CHECK-LABEL: @test_not_inverse_not_fast
define <16 x float> @test_not_inverse_not_fast(<16 x float> %val_not_fast) {
  %sqrt = call <16 x float> @llvm.sqrt.v16f32(<16 x float> %val_not_fast)
; CHECK: @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  ret <16 x float> %inv
}

; CHECK-LABEL: @test_not_inverse_double
define <16 x double> @test_not_inverse_double(<16 x double> %val_double) {
  %sqrt = call <16 x double> @llvm.sqrt.v16f64(<16 x double> %val_double)
; CHECK: @llvm.genx.inv.v16f64(<16 x double> %sqrt)
  %inv = call <16 x double> @llvm.genx.inv.v16f64(<16 x double> %sqrt)
  ret <16 x double> %inv
}

; CHECK-LABEL: @test_not_inverse_multiple_uses
define <16 x float> @test_not_inverse_multiple_uses(<16 x float> %val) {
  %sqrt = call <16 x float> @llvm.sqrt.v16f32(<16 x float> %val)
; CHECK: fadd <16 x float>
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  %add = fadd <16 x float>  %inv, %sqrt
  ret <16 x float> %add
}

; CHECK-LABEL: @test_inverse3
define <16 x float> @test_inverse3(<16 x float> %val) {
  %sqrt = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %val)
; CHECK: [[INVERSE_SQRT_FDIV:%.*]] = call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
; CHECK-NEXT: ret <16 x float> [[INVERSE_SQRT_FDIV]]
  %inv = fdiv fast <16 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %sqrt
  ret <16 x float> %inv
}

declare float @llvm.genx.sqrt.f32(float)
declare float @llvm.genx.inv.f32(float)
declare <16 x float> @llvm.sqrt.v16f32(<16 x float>)
declare <16 x double> @llvm.sqrt.v16f64(<16 x double>)
declare <16 x float> @llvm.genx.sqrt.v16f32(<16 x float>)
declare <16 x float> @llvm.genx.inv.v16f32(<16 x float>)
declare <16 x double> @llvm.genx.inv.v16f64(<16 x double>)


