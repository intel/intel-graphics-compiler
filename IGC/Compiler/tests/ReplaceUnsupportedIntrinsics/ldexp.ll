;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-17-plus
; RUN: igc_opt -igc-replace-unsupported-intrinsics -S %s 2>&1 | FileCheck %s

declare float     @llvm.ldexp.f32.i32(float %Val, i32 %Exp)
declare half    @llvm.ldexp.f16.i32(half %Val, i32 %Exp)

define float @test_ldexp_f32_i32(i32 %Exp) {
  ; CHECK-LABEL: define float @test_ldexp_f32_i32(
  ; CHECK: [[TMP1:%.*]] = sitofp i32 %Exp to float
  ; CHECK: [[TMP2:%.*]] = call float @llvm.exp2.f32(float [[TMP1]])
  ; CHECK: ret float [[TMP2]]
  %1 = call float @llvm.ldexp.f32.i32(float 1.0, i32 %Exp)
  ret float %1
}

define half @test_ldexp_f64_i32(i32 %Exp) {
  ; CHECK-LABEL: define half @test_ldexp_f64_i32(
  ; CHECK: [[TMP1:%.*]] = sitofp i32 %Exp to half
  ; CHECK: [[TMP2:%.*]] = call half @llvm.exp2.f16(half [[TMP1]])
  ; CHECK: ret half [[TMP2]]
  %1 = call half @llvm.ldexp.f16.i32(half 1.0, i32 %Exp)
  ret half %1
}

; Tests below checks if fast math flags are preserved

define float @test_ldexp_f32_i32_fmf(i32 %Exp) {
  ; CHECK-LABEL: define float @test_ldexp_f32_i32_fmf(
  ; CHECK: [[TMP1:%.*]] = sitofp i32 %Exp to float
  ; CHECK: [[TMP2:%.*]] = call fast float @llvm.exp2.f32(float [[TMP1]])
  ; CHECK: ret float [[TMP2]]
  %1 = call fast float @llvm.ldexp.f32.i32(float 1.0, i32 %Exp)
  ret float %1
}

define half @test_ldexp_f64_i32_fmf(i32 %Exp) {
  ; CHECK-LABEL: define half @test_ldexp_f64_i32_fmf(
  ; CHECK: [[TMP1:%.*]] = sitofp i32 %Exp to half
  ; CHECK: [[TMP2:%.*]] = call fast half @llvm.exp2.f16(half [[TMP1]])
  ; CHECK: ret half [[TMP2]]
  %1 = call fast half @llvm.ldexp.f16.i32(half 1.0, i32 %Exp)
  ret half %1
}

define float @test_error(float %Num, i32 %Exp) {
  ; CHECK: @llvm.ldexp with num != 1.0f argument is not supported
  %1 = call float @llvm.ldexp.f32.i32(float %Num, i32 %Exp)
  ret float %1
}