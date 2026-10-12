;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify %s -S -o - --igc-scalarizer-in-codegen 2>&1 | FileCheck %s

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Test checks fabs and abs scalarization

define <4 x bfloat> @test_fabs_scalarization_bf(<4 x bfloat> %src) {
; CHECK-LABEL: define <4 x bfloat> @test_fabs_scalarization_bf(
; CHECK-SAME: <4 x bfloat> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <4 x bfloat> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = call bfloat @llvm.fabs.bf16(bfloat [[TMP1]])
; CHECK:    [[TMP3:%.*]] = insertelement <4 x bfloat> poison, bfloat [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <4 x bfloat> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = call bfloat @llvm.fabs.bf16(bfloat [[TMP4]])
; CHECK:    [[TMP6:%.*]] = insertelement <4 x bfloat> [[TMP3]], bfloat [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <4 x bfloat> [[SRC]], i32 2
; CHECK:    [[TMP8:%.*]] = call bfloat @llvm.fabs.bf16(bfloat [[TMP7]])
; CHECK:    [[TMP9:%.*]] = insertelement <4 x bfloat> [[TMP6]], bfloat [[TMP8]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <4 x bfloat> [[SRC]], i32 3
; CHECK:    [[TMP11:%.*]] = call bfloat @llvm.fabs.bf16(bfloat [[TMP10]])
; CHECK:    [[TMP12:%.*]] = insertelement <4 x bfloat> [[TMP9]], bfloat [[TMP11]], i32 3
; CHECK:    ret <4 x bfloat> [[TMP12]]
;
  %1 = call <4 x bfloat> @llvm.fabs.v4bf16(<4 x bfloat> %src)
  ret <4 x bfloat> %1
}

define <3 x half> @test_fabs_scalarization_hf(<3 x half> %src) {
; CHECK-LABEL: define <3 x half> @test_fabs_scalarization_hf(
; CHECK-SAME: <3 x half> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <3 x half> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = call half @llvm.fabs.f16(half [[TMP1]])
; CHECK:    [[TMP3:%.*]] = insertelement <3 x half> poison, half [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <3 x half> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = call half @llvm.fabs.f16(half [[TMP4]])
; CHECK:    [[TMP6:%.*]] = insertelement <3 x half> [[TMP3]], half [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <3 x half> [[SRC]], i32 2
; CHECK:    [[TMP8:%.*]] = call half @llvm.fabs.f16(half [[TMP7]])
; CHECK:    [[TMP9:%.*]] = insertelement <3 x half> [[TMP6]], half [[TMP8]], i32 2
; CHECK:    ret <3 x half> [[TMP9]]
;
  %1 = call <3 x half> @llvm.fabs.v3f16(<3 x half> %src)
  ret <3 x half> %1
}

; Test checks that fast-math flags are preserved during scalarization

define <2 x float> @test_fabs_scalarization_fast(<2 x float> %src) {
; CHECK-LABEL: define <2 x float> @test_fabs_scalarization_fast(
; CHECK-SAME: <2 x float> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <2 x float> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = call fast float @llvm.fabs.f32(float [[TMP1]])
; CHECK:    [[TMP3:%.*]] = insertelement <2 x float> poison, float [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x float> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = call fast float @llvm.fabs.f32(float [[TMP4]])
; CHECK:    [[TMP6:%.*]] = insertelement <2 x float> [[TMP3]], float [[TMP5]], i32 1
; CHECK:    ret <2 x float> [[TMP6]]
;
  %1 = call fast <2 x float> @llvm.fabs.v2f32(<2 x float> %src)
  ret <2 x float> %1
}

define <3 x i32> @test_abs_scalarization_i32(<3 x i32> %src) {
; CHECK-LABEL: define <3 x i32> @test_abs_scalarization_i32(
; CHECK-SAME: <3 x i32> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <3 x i32> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.abs.i32(i32 [[TMP1]], i1 false)
; CHECK:    [[TMP3:%.*]] = insertelement <3 x i32> poison, i32 [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <3 x i32> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = call i32 @llvm.abs.i32(i32 [[TMP4]], i1 false)
; CHECK:    [[TMP6:%.*]] = insertelement <3 x i32> [[TMP3]], i32 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <3 x i32> [[SRC]], i32 2
; CHECK:    [[TMP8:%.*]] = call i32 @llvm.abs.i32(i32 [[TMP7]], i1 false)
; CHECK:    [[TMP9:%.*]] = insertelement <3 x i32> [[TMP6]], i32 [[TMP8]], i32 2
; CHECK:    ret <3 x i32> [[TMP9]]
;
  %1 = call <3 x i32> @llvm.abs.v3i32(<3 x i32> %src, i1 false)
  ret <3 x i32> %1
}

define <2 x i16> @test_abs_scalarization_poison(<2 x i16> %src) {
; CHECK-LABEL: define <2 x i16> @test_abs_scalarization_poison(
; CHECK-SAME: <2 x i16> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <2 x i16> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = call i16 @llvm.abs.i16(i16 [[TMP1]], i1 true)
; CHECK:    [[TMP3:%.*]] = insertelement <2 x i16> poison, i16 [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i16> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = call i16 @llvm.abs.i16(i16 [[TMP4]], i1 true)
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i16> [[TMP3]], i16 [[TMP5]], i32 1
; CHECK:    ret <2 x i16> [[TMP6]]
;
  %1 = call <2 x i16> @llvm.abs.v2i16(<2 x i16> %src, i1 true)
  ret <2 x i16> %1
}

; do nothing for scalar type

define float @test_fabs_sanity(float %src) {
; CHECK-LABEL: define float @test_fabs_sanity(
; CHECK-SAME: float [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = call float @llvm.fabs.f32(float [[SRC]])
; CHECK:    ret float [[TMP1]]
;
  %1 = call float @llvm.fabs.f32(float %src)
  ret float %1
}

declare <4 x bfloat> @llvm.fabs.v4bf16(<4 x bfloat>)
declare <3 x half> @llvm.fabs.v3f16(<3 x half>)
declare <2 x float> @llvm.fabs.v2f32(<2 x float>)
declare float @llvm.fabs.f32(float)
declare <3 x i32> @llvm.abs.v3i32(<3 x i32>, i1)
declare <2 x i16> @llvm.abs.v2i16(<2 x i16>, i1)
