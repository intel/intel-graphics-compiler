;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-NOT: fdiv
define spir_kernel void @test_fdiv(<4 x float> %arg) {
; CHECK-LABEL: @test_fdiv
; CHECK: (<4 x float> [[ARG:%[A-Za-z0-9_.]+]])
; CHECK-NEXT: [[V0:%[A-Za-z0-9_.]+]] = fmul <4 x float> [[ARG]], <
; CHECK-NEXT: [[V1:%[A-Za-z0-9_.]+]] = tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> [[V0]]
; CHECK-NEXT: [[V2:%[A-Za-z0-9_.]+]] = call <4 x float> @llvm.genx.inv.v4f32(<4 x float> [[V1]])
; CHECK-NEXT: [[V3:%[A-Za-z0-9_.]+]] = call <4 x float> @llvm.genx.rsqrt.v4f32(<4 x float> [[V1]])
; CHECK-NEXT: [[V4:%[A-Za-z0-9_.]+]] = call <4 x float> @llvm.fma.v4f32(<4 x float> [[ARG]], <4 x float> [[V2]], <4 x float> [[V3]])
; CHECK-LABEL: nextbb
; CHECK: [[V5:%[A-Za-z0-9_.]+]] = call <4 x float> @llvm.fma.v4f32(<4 x float> [[V0]], <4 x float> [[V2]], <4 x float> [[V4]])
; CHECK-NEXT: tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> [[V5]]
  %1 = fdiv <4 x float> %arg, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %2 = tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> %1, <1 x float> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %3 = fdiv arcp <4 x float> %arg, %2
  %4 = tail call fast arcp <4 x float> @llvm.sqrt.v4f32(<4 x float> %2)
  %5 = fdiv fast arcp <4 x float> <float 1.0, float 1.0, float 1.0, float 1.0>, %4
  %6 = fadd <4 x float> %3, %5
  br label %nextbb
nextbb:
  %7 = fdiv arcp <4 x float> %1, %2
  %8 = fadd <4 x float> %6, %7
  %9 = tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> %8, <1 x float> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  ret void
}

declare <4 x float> @llvm.sqrt.v4f32(<4 x float>)
declare <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float>, <1 x float>, i32, i32, i32, i16, i32, i1)