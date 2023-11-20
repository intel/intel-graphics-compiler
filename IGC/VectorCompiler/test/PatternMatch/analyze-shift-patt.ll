;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_analyze_shift_within_few_ULPs(
; CHECK-NEXT: [[CONSTANT1:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.genx.constantf.v8f32(<8 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>)
; CHECK-NEXT: [[VAL1:%[A-Za-z0-9_.]+]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> undef, <8 x float> [[CONSTANT1]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[VAL2:%[A-Za-z0-9_.]+]] = fadd <8 x float> [[CONSTANT1]], zeroinitializer
; CHECK-NEXT: [[VAL3:%[A-Za-z0-9_.]+]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> [[VAL1]], <8 x float> [[VAL2]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> [[VAL3]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
define spir_kernel void @test_analyze_shift_within_few_ULPs() {
  %wrreg1 = tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 2.8025969286496341418474591665798322625605238837530315435141365677795821653717212029732763767242431640625E-45>, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  ret void
}

; CHECK-LABEL: @test_analyze_shift_splat_value(
; CHECK-NEXT: [[CONSTANT2:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.genx.constantf.v8f32(<8 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00>)
; CHECK-NEXT: [[VAL4:%[A-Za-z0-9_.]+]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> undef, <8 x float> [[CONSTANT2]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[VAL5:%[A-Za-z0-9_.]+]] = fadd <8 x float> [[CONSTANT2]], <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>
; CHECK-NEXT: [[VAL6:%[A-Za-z0-9_.]+]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> [[VAL4]], <8 x float> [[VAL5]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> [[VAL6]], i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
define spir_kernel void @test_analyze_shift_splat_value() {
  %wrreg1 = tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 0.0, float 2.0, float 1.0, float 1.0, float 1.0, float 2.0, float 1.0, float 1.0, float 1.0>, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  ret void
}

; CHECK-LABEL: @test_analyze_shift_outside_max_ULPs(
; CHECK-NEXT: tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> <float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00>, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
define spir_kernel void @test_analyze_shift_outside_max_ULPs() {
  %wrreg1 = tail call <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float> undef, <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 0.0, float 1.0, float 0.0, float 0.0, float 1.0>, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  ret void
}

declare <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)