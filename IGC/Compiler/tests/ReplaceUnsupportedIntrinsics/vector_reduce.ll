;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S < %s | FileCheck %s

define i32 @test_reduce_and_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_and_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = and i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = and i32 [[TMP3]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = and i32 [[TMP5]], [[TMP6]]
; CHECK-NEXT:    ret i32 [[TMP7]]
;
  %r = call i32 @llvm.vector.reduce.and.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_and_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_and_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = and i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %r = call i64 @llvm.vector.reduce.and.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_or_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_or_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = or i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = or i32 [[TMP3]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = or i32 [[TMP5]], [[TMP6]]
; CHECK-NEXT:    ret i32 [[TMP7]]
;
  %r = call i32 @llvm.vector.reduce.or.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_or_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_or_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = or i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %r = call i64 @llvm.vector.reduce.or.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_xor_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_xor_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = xor i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = xor i32 [[TMP3]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = xor i32 [[TMP5]], [[TMP6]]
; CHECK-NEXT:    ret i32 [[TMP7]]
;
  %r = call i32 @llvm.vector.reduce.xor.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_xor_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_xor_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = xor i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %r = call i64 @llvm.vector.reduce.xor.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_add_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_add_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = add i32 [[TMP3]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = add i32 [[TMP5]], [[TMP6]]
; CHECK-NEXT:    ret i32 [[TMP7]]
;
  %r = call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_add_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_add_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %r = call i64 @llvm.vector.reduce.add.v2i64(<2 x i64> %v)
  ret i64 %r
}

; ============================ mul ============================

define i32 @test_reduce_mul_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_mul_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = mul i32 [[TMP3]], [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = mul i32 [[TMP5]], [[TMP6]]
; CHECK-NEXT:    ret i32 [[TMP7]]
;
  %r = call i32 @llvm.vector.reduce.mul.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_mul_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_mul_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = mul i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP3]]
;
  %r = call i64 @llvm.vector.reduce.mul.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_smax_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_smax_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp sgt i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = icmp sgt i32 [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = select i1 [[TMP6]], i32 [[TMP4]], i32 [[TMP5]]
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP9:%.*]] = icmp sgt i32 [[TMP7]], [[TMP8]]
; CHECK-NEXT:    [[TMP10:%.*]] = select i1 [[TMP9]], i32 [[TMP7]], i32 [[TMP8]]
; CHECK-NEXT:    ret i32 [[TMP10]]
;
  %r = call i32 @llvm.vector.reduce.smax.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_smax_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_smax_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp sgt i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i64 [[TMP1]], i64 [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP4]]
;
  %r = call i64 @llvm.vector.reduce.smax.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_smin_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_smin_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = icmp slt i32 [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = select i1 [[TMP6]], i32 [[TMP4]], i32 [[TMP5]]
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP9:%.*]] = icmp slt i32 [[TMP7]], [[TMP8]]
; CHECK-NEXT:    [[TMP10:%.*]] = select i1 [[TMP9]], i32 [[TMP7]], i32 [[TMP8]]
; CHECK-NEXT:    ret i32 [[TMP10]]
;
  %r = call i32 @llvm.vector.reduce.smin.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_smin_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_smin_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp slt i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i64 [[TMP1]], i64 [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP4]]
;
  %r = call i64 @llvm.vector.reduce.smin.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_umax_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_umax_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp ugt i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = icmp ugt i32 [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = select i1 [[TMP6]], i32 [[TMP4]], i32 [[TMP5]]
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP9:%.*]] = icmp ugt i32 [[TMP7]], [[TMP8]]
; CHECK-NEXT:    [[TMP10:%.*]] = select i1 [[TMP9]], i32 [[TMP7]], i32 [[TMP8]]
; CHECK-NEXT:    ret i32 [[TMP10]]
;
  %r = call i32 @llvm.vector.reduce.umax.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_umax_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_umax_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp ugt i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i64 [[TMP1]], i64 [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP4]]
;
  %r = call i64 @llvm.vector.reduce.umax.v2i64(<2 x i64> %v)
  ret i64 %r
}

define i32 @test_reduce_umin_v4i32(<4 x i32> %v) {
; CHECK-LABEL: @test_reduce_umin_v4i32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x i32> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i32> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp ult i32 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x i32> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = icmp ult i32 [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = select i1 [[TMP6]], i32 [[TMP4]], i32 [[TMP5]]
; CHECK-NEXT:    [[TMP8:%.*]] = extractelement <4 x i32> [[V]], i32 3
; CHECK-NEXT:    [[TMP9:%.*]] = icmp ult i32 [[TMP7]], [[TMP8]]
; CHECK-NEXT:    [[TMP10:%.*]] = select i1 [[TMP9]], i32 [[TMP7]], i32 [[TMP8]]
; CHECK-NEXT:    ret i32 [[TMP10]]
;
  %r = call i32 @llvm.vector.reduce.umin.v4i32(<4 x i32> %v)
  ret i32 %r
}

define i64 @test_reduce_umin_v2i64(<2 x i64> %v) {
; CHECK-LABEL: @test_reduce_umin_v2i64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i64> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = icmp ult i64 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    [[TMP4:%.*]] = select i1 [[TMP3]], i64 [[TMP1]], i64 [[TMP2]]
; CHECK-NEXT:    ret i64 [[TMP4]]
;
  %r = call i64 @llvm.vector.reduce.umin.v2i64(<2 x i64> %v)
  ret i64 %r
}

define float @test_reduce_fadd_v4f32(float %start, <4 x float> %v) {
; CHECK-LABEL: @test_reduce_fadd_v4f32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x float> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = fadd float [[START:%.*]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK-NEXT:    [[TMP4:%.*]] = fadd float [[TMP2]], [[TMP3]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = fadd float [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK-NEXT:    [[TMP8:%.*]] = fadd float [[TMP6]], [[TMP7]]
; CHECK-NEXT:    ret float [[TMP8]]
;
  %r = call float @llvm.vector.reduce.fadd.v4f32(float %start, <4 x float> %v)
  ret float %r
}

define double @test_reduce_fadd_v2f64(double %start, <2 x double> %v) {
; CHECK-LABEL: @test_reduce_fadd_v2f64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x double> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = fadd double [[START:%.*]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <2 x double> [[V]], i32 1
; CHECK-NEXT:    [[TMP4:%.*]] = fadd double [[TMP2]], [[TMP3]]
; CHECK-NEXT:    ret double [[TMP4]]
;
  %r = call double @llvm.vector.reduce.fadd.v2f64(double %start, <2 x double> %v)
  ret double %r
}

define float @test_reduce_fmul_v4f32(float %start, <4 x float> %v) {
; CHECK-LABEL: @test_reduce_fmul_v4f32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x float> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = fmul float [[START:%.*]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK-NEXT:    [[TMP4:%.*]] = fmul float [[TMP2]], [[TMP3]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK-NEXT:    [[TMP6:%.*]] = fmul float [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK-NEXT:    [[TMP8:%.*]] = fmul float [[TMP6]], [[TMP7]]
; CHECK-NEXT:    ret float [[TMP8]]
;
  %r = call float @llvm.vector.reduce.fmul.v4f32(float %start, <4 x float> %v)
  ret float %r
}

define double @test_reduce_fmul_v2f64(double %start, <2 x double> %v) {
; CHECK-LABEL: @test_reduce_fmul_v2f64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x double> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = fmul double [[START:%.*]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <2 x double> [[V]], i32 1
; CHECK-NEXT:    [[TMP4:%.*]] = fmul double [[TMP2]], [[TMP3]]
; CHECK-NEXT:    ret double [[TMP4]]
;
  %r = call double @llvm.vector.reduce.fmul.v2f64(double %start, <2 x double> %v)
  ret double %r
}

define float @test_reduce_fmax_v4f32(<4 x float> %v) {
; CHECK-LABEL: @test_reduce_fmax_v4f32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x float> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = call float @llvm.maxnum.f32(float [[TMP1]], float [[TMP2]])
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = call float @llvm.maxnum.f32(float [[TMP3]], float [[TMP4]])
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = call float @llvm.maxnum.f32(float [[TMP5]], float [[TMP6]])
; CHECK-NEXT:    ret float [[TMP7]]
;
  %r = call float @llvm.vector.reduce.fmax.v4f32(<4 x float> %v)
  ret float %r
}

define double @test_reduce_fmax_v2f64(<2 x double> %v) {
; CHECK-LABEL: @test_reduce_fmax_v2f64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x double> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x double> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = call double @llvm.maxnum.f64(double [[TMP1]], double [[TMP2]])
; CHECK-NEXT:    ret double [[TMP3]]
;
  %r = call double @llvm.vector.reduce.fmax.v2f64(<2 x double> %v)
  ret double %r
}

define float @test_reduce_fmin_v4f32(<4 x float> %v) {
; CHECK-LABEL: @test_reduce_fmin_v4f32(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <4 x float> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = call float @llvm.minnum.f32(float [[TMP1]], float [[TMP2]])
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK-NEXT:    [[TMP5:%.*]] = call float @llvm.minnum.f32(float [[TMP3]], float [[TMP4]])
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK-NEXT:    [[TMP7:%.*]] = call float @llvm.minnum.f32(float [[TMP5]], float [[TMP6]])
; CHECK-NEXT:    ret float [[TMP7]]
;
  %r = call float @llvm.vector.reduce.fmin.v4f32(<4 x float> %v)
  ret float %r
}

define double @test_reduce_fmin_v2f64(<2 x double> %v) {
; CHECK-LABEL: @test_reduce_fmin_v2f64(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x double> [[V:%.*]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x double> [[V]], i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = call double @llvm.minnum.f64(double [[TMP1]], double [[TMP2]])
; CHECK-NEXT:    ret double [[TMP3]]
;
  %r = call double @llvm.vector.reduce.fmin.v2f64(<2 x double> %v)
  ret double %r
}

declare i32 @llvm.vector.reduce.and.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.and.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.or.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.or.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.xor.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.xor.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.add.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.mul.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.mul.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.smax.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.smax.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.smin.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.smin.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.umax.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.umax.v2i64(<2 x i64>)
declare i32 @llvm.vector.reduce.umin.v4i32(<4 x i32>)
declare i64 @llvm.vector.reduce.umin.v2i64(<2 x i64>)
declare float @llvm.vector.reduce.fadd.v4f32(float, <4 x float>)
declare double @llvm.vector.reduce.fadd.v2f64(double, <2 x double>)
declare float @llvm.vector.reduce.fmul.v4f32(float, <4 x float>)
declare double @llvm.vector.reduce.fmul.v2f64(double, <2 x double>)
declare float @llvm.vector.reduce.fmax.v4f32(<4 x float>)
declare double @llvm.vector.reduce.fmax.v2f64(<2 x double>)
declare float @llvm.vector.reduce.fmin.v4f32(<4 x float>)
declare double @llvm.vector.reduce.fmin.v2f64(<2 x double>)
