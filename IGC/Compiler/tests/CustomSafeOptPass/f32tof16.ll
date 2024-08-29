;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: GenISA.f32tof16.rtz
; ------------------------------------------------

; Test checks that 2 GenISA.f32tof16.rtz intrinsics moved to low and high part of i32
; are substitued with GenISA.ftof.rtz.f16.f32 with insertelements

define i32 @test_f32tof16_shl(float %src1, float %src2) {
; CHECK-LABEL: define i32 @test_f32tof16_shl(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP6:%.*]] = call half @llvm.genx.GenISA.ftof.rtz.f16.f32(float [[SRC2]])
; CHECK:    [[TMP7:%.*]] = call half @llvm.genx.GenISA.ftof.rtz.f16.f32(float [[SRC1]])
; CHECK:    [[TMP8:%.*]] = insertelement <2 x half> undef, half [[TMP6]], i32 0
; CHECK:    [[TMP9:%.*]] = insertelement <2 x half> [[TMP8]], half [[TMP7]], i32 1
; CHECK:    [[TMP10:%.*]] = bitcast <2 x half> [[TMP9]] to i32
; CHECK:    ret i32 [[TMP10]]
;
  %1 = call float @llvm.genx.GenISA.f32tof16.rtz(float %src1)
  %2 = call float @llvm.genx.GenISA.f32tof16.rtz(float %src2)
  %3 = bitcast float %1 to i32
  %4 = bitcast float %2 to i32
  %5 = shl i32 %3, 16
  %6 = add i32 %5, %4
  ret i32 %6
}

define i32 @test_f32tof16_mul(float %src1, float %src2) {
; CHECK-LABEL: define i32 @test_f32tof16_mul(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP6:%.*]] = call half @llvm.genx.GenISA.ftof.rtz.f16.f32(float [[SRC2]])
; CHECK:    [[TMP7:%.*]] = call half @llvm.genx.GenISA.ftof.rtz.f16.f32(float [[SRC1]])
; CHECK:    [[TMP8:%.*]] = insertelement <2 x half> undef, half [[TMP6]], i32 0
; CHECK:    [[TMP9:%.*]] = insertelement <2 x half> [[TMP8]], half [[TMP7]], i32 1
; CHECK:    [[TMP10:%.*]] = bitcast <2 x half> [[TMP9]] to i32
; CHECK:    ret i32 [[TMP10]]
;
  %1 = call float @llvm.genx.GenISA.f32tof16.rtz(float %src1)
  %2 = call float @llvm.genx.GenISA.f32tof16.rtz(float %src2)
  %3 = bitcast float %1 to i32
  %4 = bitcast float %2 to i32
  %5 = mul i32 %3, 65536
  %6 = add i32 %5, %4
  ret i32 %6
}

; If value was produced by fpext from half
; then trasformation discards conversion call

define i32 @test_f32tof16_half(half %src1, half %src2) {
; CHECK-LABEL: define i32 @test_f32tof16_half(
; CHECK-SAME: half [[SRC1:%.*]], half [[SRC2:%.*]]) {
; CHECK:    [[TMP8:%.*]] = insertelement <2 x half> undef, half [[SRC2]], i32 0
; CHECK:    [[TMP9:%.*]] = insertelement <2 x half> [[TMP8]], half [[SRC1]], i32 1
; CHECK:    [[TMP10:%.*]] = bitcast <2 x half> [[TMP9]] to i32
; CHECK:    ret i32 [[TMP10]]
;
  %1 = fpext half %src1 to float
  %2 = fpext half %src2 to float
  %3 = call float @llvm.genx.GenISA.f32tof16.rtz(float %1)
  %4 = call float @llvm.genx.GenISA.f32tof16.rtz(float %2)
  %5 = bitcast float %3 to i32
  %6 = bitcast float %4 to i32
  %7 = mul i32 %5, 65536
  %8 = add i32 %7, %6
  ret i32 %8
}

declare float @llvm.genx.GenISA.f32tof16.rtz(float)
