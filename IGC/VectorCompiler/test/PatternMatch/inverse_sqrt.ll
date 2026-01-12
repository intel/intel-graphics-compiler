;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe2 \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

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

; CHECK-LABEL: @test_inverse_not_fast
define <16 x float> @test_inverse_not_fast(<16 x float> %src) {
; CHECK: @llvm.genx.rsqrt.v16f32(<16 x float> %src)
  %sqrt = call <16 x float> @llvm.sqrt.v16f32(<16 x float> %src)
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  ret <16 x float> %inv
}

; CHECK-LABEL: @test_inverse_double
define <16 x double> @test_inverse_double(<16 x double> %val_double) {
  %sqrt = call <16 x double> @llvm.sqrt.v16f64(<16 x double> %val_double)
; CHECK: call <16 x double> @llvm.genx.rsqrt.v16f64(<16 x double> %val_double)
  %inv = call <16 x double> @llvm.genx.inv.v16f64(<16 x double> %sqrt)
  ret <16 x double> %inv
}

; CHECK-LABEL: @test_not_inverse_multiple_uses
define <16 x float> @test_not_inverse_multiple_uses(<16 x float> %val) {
; CHECK: call <16 x float> @llvm.sqrt.v16f32(<16 x float> %val)
; CHECK: call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
; CHECK: fadd <16 x float>
  %sqrt = call <16 x float> @llvm.sqrt.v16f32(<16 x float> %val)
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

; CHECK-LABEL: @test_inverse_of_not_sqrt
define <16 x float> @test_inverse_of_not_sqrt(<16 x float> %val, <16 x float> %tmp) {
; CHECK-NEXT: fsub <16 x float>
; CHECK-NEXT: call <16 x float> @llvm.genx.inv.v16f32
  %sub = fsub <16 x float> %val, %tmp
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sub)
  ret <16 x float> %inv
}

; CHECK-LABEL: @test_sqrt_multiple_use
define <16 x float> @test_sqrt_multiple_use(<16 x float> %val) {
; CHECK-NEXT: call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %val)
; CHECK-NEXT: call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
; CHECK-NEXT: fadd <16 x float>
  %sqrt = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %val)
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %sqrt)
  %add = fadd <16 x float> %sqrt, %inv
  ret <16 x float> %add
}

; CHECK-LABEL: @test_sqrt_inv_constant
define float @test_sqrt_inv_constant() {
; CHECK: ret float 0x3FB99999A0000000
  %sqrt = call float @llvm.genx.sqrt.f32(float 100.0)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_div_constant
define float @test_sqrt_div_constant() {
; CHECK: ret float 0x3FB99999A0000000
  %sqrt = call float @llvm.genx.sqrt.f32(float 100.0)
  %inv = fdiv arcp float 1.0, %sqrt
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_fast_inv_constant
define float @test_sqrt_fast_inv_constant() {
; CHECK: ret float 0x3FB99999A0000000
  %sqrt = call afn float @llvm.sqrt.f32(float 100.0)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_neg
define float @test_sqrt_inv_constant_neg() {
; CHECK: ret float 0x7FF8000000000000
  %sqrt = call float @llvm.genx.sqrt.f32(float -100.0)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_negzero
define float @test_sqrt_inv_constant_negzero() {
; CHECK: ret float 0xFFF0000000000000
  %sqrt = call float @llvm.genx.sqrt.f32(float -0.0)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_zero
define float @test_sqrt_inv_constant_zero() {
; CHECK: ret float 0x7FF0000000000000
  %sqrt = call float @llvm.genx.sqrt.f32(float 0.0)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_inf
define float @test_sqrt_inv_constant_inf() {
; CHECK: ret float 0.000000e+00
  %sqrt = call float @llvm.genx.sqrt.f32(float 0x7FF0000000000000)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_neginf
define float @test_sqrt_inv_constant_neginf() {
; CHECK: ret float 0x7FF8000000000000
  %sqrt = call float @llvm.genx.sqrt.f32(float 0xFFF0000000000000)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_undef
define float @test_sqrt_inv_constant_undef() {
; CHECK: ret float undef
  %sqrt = call float @llvm.genx.sqrt.f32(float undef)
  %inv = call float @llvm.genx.inv.f32(float %sqrt)
  ret float %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_vector
define <2 x float> @test_sqrt_inv_constant_vector() {
; CHECK: ret <2 x float> <float 0x3FC99999A0000000, float 5.000000e-01>
  %sqrt = call <2 x float> @llvm.genx.sqrt.v2f32(<2 x float> <float 25.0, float 4.0>)
  %inv = call <2 x float> @llvm.genx.inv.v2f32(<2 x float> %sqrt)
  ret <2 x float> %inv
}

; CHECK-LABEL: @test_sqrt_inv_constant_vector_zero
define <2 x float> @test_sqrt_inv_constant_vector_zero() {
; CHECK: ret <2 x float> <float 0x7FF0000000000000, float 0x7FF0000000000000>
  %sqrt = call <2 x float> @llvm.genx.sqrt.v2f32(<2 x float> zeroinitializer)
  %inv = call <2 x float> @llvm.genx.inv.v2f32(<2 x float> %sqrt)
  ret <2 x float> %inv
}

; CHECK-LABEL: @test_inv_sqrt_1
define <16 x float> @test_inv_sqrt_1(<16 x float> %val) {
; CHECK: call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %val)
  %sqrt = call <16 x float> @llvm.genx.sqrt.v16f32(<16 x float> %inv)
  ret <16 x float> %sqrt
}

; CHECK-LABEL: @test_inv_sqrt_2
define <16 x float> @test_inv_sqrt_2(<16 x float> %val) {
; CHECK: call <16 x float> @llvm.genx.rsqrt.v16f32(<16 x float> %val)
  %inv = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %val)
  %sqrt = call <16 x float> @llvm.sqrt.v16f32(<16 x float> %inv)
  ret <16 x float> %sqrt
}

; CHECK-LABEL: @test_inv_sqrt_3
define float @test_inv_sqrt_3(float %val) {
; CHECK: call float @llvm.genx.rsqrt.f32(float %val)
  %inv = fdiv float 1.0, %val
  %sqrt = call float @llvm.genx.sqrt.f32(float %inv)
  ret float %sqrt
}

; CHECK-LABEL: @test_inv_sqrt_4
define float @test_inv_sqrt_4(float %val) {
; CHECK: call float @llvm.genx.rsqrt.f32(float %val)
  %inv = fdiv float 1.0, %val
  %sqrt = call afn float @llvm.sqrt.f32(float %inv)
  ret float %sqrt
}

; CHECK-LABEL: @test_inv_sqrt_5
define float @test_inv_sqrt_5(float %val) {
; CHECK: call float @llvm.genx.rsqrt.f32(float %val)
  %inv = fdiv arcp float 1.0, %val
  %sqrt = call float @llvm.sqrt.f32(float %inv)
  ret float %sqrt
}

; CHECK-LABEL: @test_inv_sqrt_6
define float @test_inv_sqrt_6(float %val) {
; CHECK-NOT: call float @llvm.genx.rsqrt.f32(float %val)
  %inv = fdiv float 1.0, %val
  %sqrt = call float @llvm.sqrt.f32(float %inv)
  ret float %sqrt
}

; CHECK-LABEL: @test_inverse_double_2
define <16 x double> @test_inverse_double_2(<16 x double> %val_double) {
  %sqrt = call <16 x double> @llvm.genx.sqrt.v16f64(<16 x double> %val_double)
  %div = fdiv <16 x double> <double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00>, %sqrt
; CHECK: call <16 x double> @llvm.genx.rsqrt.v16f64(<16 x double> %val_double)
  ret <16 x double> %div
}

; CHECK-LABEL: @test_inverse_double_3
define <16 x double> @test_inverse_double_3(<16 x double> %val_double) {
  %div = fdiv arcp <16 x double> <double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00>, %val_double
  %sqrt = call <16 x double> @llvm.genx.sqrt.v16f64(<16 x double> %div)
; CHECK: call <16 x double> @llvm.genx.rsqrt.v16f64(<16 x double> %val_double)
  ret <16 x double> %sqrt
}

declare float @llvm.sqrt.f32(float)
declare float @llvm.genx.sqrt.f32(float)
declare float @llvm.genx.inv.f32(float)
declare <2 x float> @llvm.genx.sqrt.v2f32(<2 x float>)
declare <2 x float> @llvm.genx.inv.v2f32(<2 x float>)
declare <16 x float> @llvm.sqrt.v16f32(<16 x float>)
declare <16 x double> @llvm.sqrt.v16f64(<16 x double>)
declare <16 x float> @llvm.genx.sqrt.v16f32(<16 x float>)
declare <16 x double> @llvm.genx.sqrt.v16f64(<16 x double>)
declare <16 x float> @llvm.genx.inv.v16f32(<16 x float>)
declare <16 x double> @llvm.genx.inv.v16f64(<16 x double>)
