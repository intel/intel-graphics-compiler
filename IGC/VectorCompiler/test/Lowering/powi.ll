;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare float @llvm.powi.f32.i64(float, i64) #2
; CHECK-LABEL: powi_float_i64
define spir_func float @powi_float_i64(i64 %src) {
; CHECK: {{[V0-9]*}} = call afn float @llvm.pow.f32
  %res = call afn float @llvm.powi.f32.i64(float 0x4000000000000000, i64 %src)
  ret float %res
}

declare float @llvm.powi.f32.i8(float, i8) #2
; CHECK-LABEL: powi_float_i8
define spir_func float @powi_float_i8(i8 %src) {
; CHECK: {{[V0-9]*}} = call afn float @llvm.pow.f32
  %res = call afn float @llvm.powi.f32.i8(float 0x4000000000000000, i8 %src)
  ret float %res
}

declare <4 x double> @llvm.powi.v4f64.v4i64(<4 x double>, <4 x i64>) #2
; CHECK-LABEL: powi_double_i64
define spir_func <4 x double> @powi_double_i64(<4 x i64> %src, <4 x double> %src_dbl) {
; CHECK: {{[V0-9]*}} = call afn <4 x double> @llvm.pow.v4f64
  %res = call afn <4 x double> @llvm.powi.v4f64.v4i64(<4 x double> %src_dbl, <4 x i64> %src)
  ret <4 x double> %res
}

declare <8 x double> @llvm.powi.v8f64.v8i8(<8 x double>, <8 x i8>) #2
; CHECK-LABEL: powi_double_i8
define spir_func <8 x double> @powi_double_i8(<8 x i8> %src, <8 x double> %src_dbl) {
; CHECK: {{[V0-9]*}} = call afn <8 x double> @llvm.pow.v8f64
  %res = call afn <8 x double> @llvm.powi.v8f64.v8i8(<8 x double> %src_dbl, <8 x i8> %src)
  ret <8 x double> %res
}

declare bfloat @llvm.powi.bf16.i64(bfloat, i64) #2
; CHECK-LABEL: powi_bfloat_i64
define spir_func bfloat @powi_bfloat_i64(i64 %src) {
; CHECK: {{[V0-9]*}} = call afn bfloat @llvm.pow.bf16
  %res = call afn bfloat @llvm.powi.bf16.i64(bfloat 0x4000000000000000, i64 %src)
  ret bfloat %res
}

declare <64 x bfloat> @llvm.powi.v64bf16.v64i8(<64 x bfloat>, <64 x i8>) #2
; CHECK-LABEL: powi_bfloat_i8
define spir_func <64 x bfloat> @powi_bfloat_i8(<64 x i8> %src, <64 x bfloat> %src_bfl) {
; CHECK: {{[V0-9]*}} = call afn <64 x bfloat> @llvm.pow.v64bf16
  %res = call afn <64 x bfloat> @llvm.powi.v64bf16.v64i8(<64 x bfloat> %src_bfl, <64 x i8> %src)
  ret <64 x bfloat> %res
}

declare <64 x float> @llvm.powi.v64f32.i32(<64 x float>, i32)
; CHECK-LABEL: powi_float_i32
define spir_func <64 x float> @powi_float_i32(<64 x float> %src) {
; CHECK: {{.*}} = call afn <64 x float> @llvm.pow.v64f32(<64 x float> %src, <64 x float> <float -4.000000e+00,
  %res = tail call afn <64 x float> @llvm.powi.v64f32.i32(<64 x float> %src, i32 -4)
  ret <64 x float> %res
}

attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
