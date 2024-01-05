;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s --check-prefix=Gen9
; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Xe2 -mtriple=spir64 -S < %s | FileCheck %s --check-prefix=Xe2

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"
define void @test(<4 x float> %bti) {
  %div = fdiv <4 x float> %bti, <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>
  ret void
}

; Gen9-LABEL: @test
; Gen9-NEXT: [[DIV:%[^ ]+]] = fdiv <4 x float> %bti, <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>

; Xe2-LABEL: @test
; Xe2-NEXT: [[CST0:%[^ ]+]] = call <1 x float> @llvm.genx.constantf.v1f32(<1 x float> <float 4.000000e+00>)
; Xe2-NEXT: [[CST1:%[^ ]+]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v1f32.i16(<1 x float> [[CST0]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; Xe2-NEXT: [[CST2:%[^ ]+]] = call <4 x float> @llvm.genx.wrconstregion.v4f32.v1f32.i16.i1(<4 x float> [[CST1]], <1 x float> <float 3.000000e+00>, i32 1, i32 1, i32 1, i16 8, i32 undef, i1 true)
; Xe2-NEXT: [[CST3:%[^ ]+]] = call <4 x float> @llvm.genx.wrconstregion.v4f32.v1f32.i16.i1(<4 x float> [[CST2]], <1 x float> <float 2.000000e+00>, i32 1, i32 1, i32 1, i16 4, i32 undef, i1 true)
; Xe2-NEXT: [[CST4:%[^ ]+]] = call <4 x float> @llvm.genx.wrconstregion.v4f32.v1f32.i16.i1(<4 x float> [[CST3]], <1 x float> <float 1.000000e+00>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
; Xe2-NEXT: [[DIV:%[^ ]+]] = fdiv <4 x float> %bti, [[CST4]]
