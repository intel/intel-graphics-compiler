;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix CHECK-NO-IEEE

declare double @llvm.minnum.f64(double, double)
declare double @llvm.maxnum.f64(double, double)

; CHECK-LABEL: min_max
define internal spir_func void @min_max(double %arg1, double %arg2) {
; CHECK: @llvm.genx.fmin.f64.f64(double %arg1, double %arg2)
  %1 = call double @llvm.minnum.f64(double %arg1, double %arg2)
; CHECK: @llvm.genx.fmax.f64.f64(double %arg1, double %arg2)
  %2 = call double @llvm.maxnum.f64(double %arg1, double %arg2)
  ret void
}

declare <8 x double> @llvm.sqrt.v8f64(<8 x double>)
declare <8 x float> @llvm.sqrt.v8f32(<8 x float>)

define <8 x double> @test_sqrt_double(<8 x double> %a0) {
; CHECK:  [[IEEE_SQRT_DOUBLE1:%.*]] = call <8 x double> @llvm.genx.ieee.sqrt.v8f64(<8 x double> %a0)
; CHECK-NEXT: ret <8 x double> [[IEEE_SQRT_DOUBLE1]]
  %1 = call <8 x double> @llvm.sqrt.v8f64(<8 x double> %a0)
  ret <8 x double> %1
}

define <8 x double> @test_sqrt_fast_double(<8 x double> %a0) {
; COM: Generating @llvm.genx.ieee.sqrt since @llvm.genx.sqrt cannot take double.
; CHECK:  [[IEEE_SQRT_DOUBLE2:%.*]] = call <8 x double> @llvm.genx.ieee.sqrt.v8f64(<8 x double> %a0)
; CHECK-NEXT: ret <8 x double> [[IEEE_SQRT_DOUBLE2]]
  %1 = call fast <8 x double> @llvm.sqrt.v8f64(<8 x double> %a0)
  ret <8 x double> %1
}

define <8 x double> @test_sqrt_afn_double(<8 x double> %a0) {
; COM: Generating @llvm.genx.ieee.sqrt since @llvm.genx.sqrt cannot take double.
; CHECK:  [[IEEE_SQRT_DOUBLE3:%.*]] = call <8 x double> @llvm.genx.ieee.sqrt.v8f64(<8 x double> %a0)
; CHECK-NEXT: ret <8 x double> [[IEEE_SQRT_DOUBLE3]]
  %1 = call afn <8 x double> @llvm.sqrt.v8f64(<8 x double> %a0)
  ret <8 x double> %1
}

define <8 x float> @test_sqrt_not_afn(<8 x float> %a0) {
; CHECK:  [[IEEE_SQRT_FLOAT:%.*]] = call <8 x float> @llvm.genx.ieee.sqrt.v8f32(<8 x float> %a0)
; CHECK-NEXT:  ret <8 x float> [[IEEE_SQRT_FLOAT]]
; CHECK-NO-IEEE:  [[IEEE_SQRT_FLOAT:%.*]] = call <8 x float> @llvm.genx.sqrt.v8f32(<8 x float> %a0)
; CHECK-NO-IEEE-NEXT:  ret <8 x float> [[IEEE_SQRT_FLOAT]]
  %1 = call <8 x float> @llvm.sqrt.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

define <8 x float> @test_sqrt_fast(<8 x float> %a0) {
; CHECK:  [[NATIVE_SQRT:%.*]] = call <8 x float> @llvm.genx.sqrt.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_SQRT]]
  %1 = call fast <8 x float> @llvm.sqrt.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

define <8 x float> @test_sqrt_afn(<8 x float> %a0) {
; CHECK:  [[NATIVE_SQRT2:%.*]] = call <8 x float> @llvm.genx.sqrt.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_SQRT2]]
  %1 = call afn <8 x float> @llvm.sqrt.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

declare <8 x float> @llvm.cos.v8f32(<8 x float>)

define <8 x float> @test_cos_afn(<8 x float> %a0) {
; CHECK:  [[NATIVE_COS:%.*]] = call <8 x float> @llvm.genx.cos.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_COS]]
  %1 = call afn <8 x float> @llvm.cos.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

declare <8 x float> @llvm.exp2.v8f32(<8 x float>)

define <8 x float> @test_exp2_afn(<8 x float> %a0) {
; CHECK:  [[NATIVE_EXP:%.*]] = call <8 x float> @llvm.genx.exp.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_EXP]]
  %1 = call afn <8 x float> @llvm.exp2.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

declare <8 x float> @llvm.log2.v8f32(<8 x float>)

define <8 x float> @test_log2_afn(<8 x float> %a0) {
; CHECK:  [[NATIVE_LOG:%.*]] = call <8 x float> @llvm.genx.log.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_LOG]]
  %1 = call afn <8 x float> @llvm.log2.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

declare <8 x float> @llvm.sin.v8f32(<8 x float>)

define <8 x float> @test_sin_afn(<8 x float> %a0) {
; CHECK:  [[NATIVE_SIN:%.*]] = call <8 x float> @llvm.genx.sin.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_SIN]]
  %1 = call afn <8 x float> @llvm.sin.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}

declare <8 x float> @llvm.ceil.v8f32(<8 x float>)

define <8 x float> @test_ceil(<8 x float> %a0) {
; CHECK:  [[NATIVE_CEIL:%.*]] = call <8 x float> @llvm.genx.rndu.v8f32(<8 x float> %a0)
; CHECK-NEXT: ret <8 x float> [[NATIVE_CEIL]]
  %1 = call <8 x float> @llvm.ceil.v8f32(<8 x float> %a0)
  ret <8 x float> %1
}
