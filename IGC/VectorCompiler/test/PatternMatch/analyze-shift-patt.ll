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

; CHECK-LABEL: @test_analyze_shift_Expr(
; CHECK-NEXT: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64
define spir_kernel void @test_analyze_shift_Expr(i64 %in1) {
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %in1, i16 1, i32 0, <16 x i64> bitcast (<32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31> to <16 x i64>))

  ret void
}

declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i64>)

declare <16 x float> @llvm.genx.wrregionf.v16f32.v16f32.i16.i1(<16 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)
