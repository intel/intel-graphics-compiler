;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; due to bfloat ty
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Checks legalization of copysign intrinsic
;

define float @test_copysign_f32(float %s1, float %s2) {
; CHECK-LABEL: define float @test_copysign_f32(
; CHECK-SAME: float [[S1:%.*]], float [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast float [[S1]] to i32
; CHECK:    [[TMP2:%.*]] = bitcast float [[S2]] to i32
; CHECK:    [[TMP3:%.*]] = and i32 [[TMP1]], 2147483647
; CHECK:    [[TMP4:%.*]] = and i32 [[TMP2]], -2147483648
; CHECK:    [[TMP5:%.*]] = or i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = bitcast i32 [[TMP5]] to float
; CHECK:    ret float [[TMP6]]
;
  %1 = call float @llvm.copysign.f32(float %s1, float %s2)
  ret float %1
}

define half @test_copysign_f16(half %s1, half %s2) {
; CHECK-LABEL: define half @test_copysign_f16(
; CHECK-SAME: half [[S1:%.*]], half [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast half [[S1]] to i16
; CHECK:    [[TMP2:%.*]] = bitcast half [[S2]] to i16
; CHECK:    [[TMP3:%.*]] = and i16 [[TMP1]], 32767
; CHECK:    [[TMP4:%.*]] = and i16 [[TMP2]], -32768
; CHECK:    [[TMP5:%.*]] = or i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = bitcast i16 [[TMP5]] to half
; CHECK:    ret half [[TMP6]]
;
  %1 = call half @llvm.copysign.f16(half %s1, half %s2)
  ret half %1
}

define bfloat @test_copysign_bf(bfloat %s1, bfloat %s2) {
; CHECK-LABEL: define bfloat @test_copysign_bf(
; CHECK-SAME: bfloat [[S1:%.*]], bfloat [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast bfloat [[S1]] to i16
; CHECK:    [[TMP2:%.*]] = bitcast bfloat [[S2]] to i16
; CHECK:    [[TMP3:%.*]] = and i16 [[TMP1]], 32767
; CHECK:    [[TMP4:%.*]] = and i16 [[TMP2]], -32768
; CHECK:    [[TMP5:%.*]] = or i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = bitcast i16 [[TMP5]] to bfloat
; CHECK:    ret bfloat [[TMP6]]
;
  %1 = call bfloat @llvm.copysign.bf(bfloat %s1, bfloat %s2)
  ret bfloat %1
}

define double @test_copysign_f64(double %s1, double %s2) {
; CHECK-LABEL: define double @test_copysign_f64(
; CHECK-SAME: double [[S1:%.*]], double [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast double [[S1]] to i64
; CHECK:    [[TMP2:%.*]] = bitcast double [[S2]] to i64
; CHECK:    [[TMP3:%.*]] = and i64 [[TMP1]], 9223372036854775807
; CHECK:    [[TMP4:%.*]] = and i64 [[TMP2]], -9223372036854775808
; CHECK:    [[TMP5:%.*]] = or i64 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = bitcast i64 [[TMP5]] to double
; CHECK:    ret double [[TMP6]]
;
  %1 = call double @llvm.copysign.f64(double %s1, double %s2)
  ret double %1
}

define <4 x float> @test_copysign_v4f32(<4 x float> %s1, <4 x float> %s2) {
; CHECK-LABEL: define <4 x float> @test_copysign_v4f32(
; CHECK-SAME: <4 x float> [[S1:%.*]], <4 x float> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x float> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = bitcast float [[TMP1]] to i32
; CHECK:    [[TMP4:%.*]] = bitcast float [[TMP2]] to i32
; CHECK:    [[TMP5:%.*]] = and i32 [[TMP3]], 2147483647
; CHECK:    [[TMP6:%.*]] = and i32 [[TMP4]], -2147483648
; CHECK:    [[TMP7:%.*]] = or i32 [[TMP5]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = bitcast i32 [[TMP7]] to float
; CHECK:    [[TMP9:%.*]] = insertelement <4 x float> undef, float [[TMP8]], i64 0
; CHECK:    [[TMP10:%.*]] = extractelement <4 x float> [[S1]], i64 1
; CHECK:    [[TMP11:%.*]] = extractelement <4 x float> [[S2]], i64 1
; CHECK:    [[TMP12:%.*]] = bitcast float [[TMP10]] to i32
; CHECK:    [[TMP13:%.*]] = bitcast float [[TMP11]] to i32
; CHECK:    [[TMP14:%.*]] = and i32 [[TMP12]], 2147483647
; CHECK:    [[TMP15:%.*]] = and i32 [[TMP13]], -2147483648
; CHECK:    [[TMP16:%.*]] = or i32 [[TMP14]], [[TMP15]]
; CHECK:    [[TMP17:%.*]] = bitcast i32 [[TMP16]] to float
; CHECK:    [[TMP18:%.*]] = insertelement <4 x float> [[TMP9]], float [[TMP17]], i64 1
; CHECK:    [[TMP19:%.*]] = extractelement <4 x float> [[S1]], i64 2
; CHECK:    [[TMP20:%.*]] = extractelement <4 x float> [[S2]], i64 2
; CHECK:    [[TMP21:%.*]] = bitcast float [[TMP19]] to i32
; CHECK:    [[TMP22:%.*]] = bitcast float [[TMP20]] to i32
; CHECK:    [[TMP23:%.*]] = and i32 [[TMP21]], 2147483647
; CHECK:    [[TMP24:%.*]] = and i32 [[TMP22]], -2147483648
; CHECK:    [[TMP25:%.*]] = or i32 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP26:%.*]] = bitcast i32 [[TMP25]] to float
; CHECK:    [[TMP27:%.*]] = insertelement <4 x float> [[TMP18]], float [[TMP26]], i64 2
; CHECK:    [[TMP28:%.*]] = extractelement <4 x float> [[S1]], i64 3
; CHECK:    [[TMP29:%.*]] = extractelement <4 x float> [[S2]], i64 3
; CHECK:    [[TMP30:%.*]] = bitcast float [[TMP28]] to i32
; CHECK:    [[TMP31:%.*]] = bitcast float [[TMP29]] to i32
; CHECK:    [[TMP32:%.*]] = and i32 [[TMP30]], 2147483647
; CHECK:    [[TMP33:%.*]] = and i32 [[TMP31]], -2147483648
; CHECK:    [[TMP34:%.*]] = or i32 [[TMP32]], [[TMP33]]
; CHECK:    [[TMP35:%.*]] = bitcast i32 [[TMP34]] to float
; CHECK:    [[TMP36:%.*]] = insertelement <4 x float> [[TMP27]], float [[TMP35]], i64 3
; CHECK:    ret <4 x float> [[TMP36]]
;
  %1 = call <4 x float> @llvm.copysign.v4f32(<4 x float> %s1, <4 x float> %s2)
  ret <4 x float> %1
}

declare float @llvm.copysign.f32(float, float)
declare half @llvm.copysign.f16(half, half)
declare bfloat @llvm.copysign.bf(bfloat, bfloat)
declare double @llvm.copysign.f64(double, double)
declare <4 x float> @llvm.copysign.v4f32(<4 x float>, <4 x float>)

!igc.functions = !{!0, !1, !2, !3, !4}

!0 = !{float (float, float)* @test_copysign_f32, !5}
!1 = !{half (half, half)* @test_copysign_f16, !5}
!2 = !{bfloat (bfloat, bfloat)* @test_copysign_bf, !5}
!3 = !{double (double, double)* @test_copysign_f64, !5}
!4 = !{<4 x float> (<4 x float>, <4 x float>)* @test_copysign_v4f32, !5}
!5 = !{}
