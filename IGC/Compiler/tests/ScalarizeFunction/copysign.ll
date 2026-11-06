;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------

define <3 x half> @test_copysign_v3f16(<3 x half> %mag, <3 x half> %sign) {
; CHECK-LABEL: @test_copysign_v3f16(
; CHECK-DAG:     [[SIGN0:%.*]] = extractelement <3 x half> %sign, i32 0
; CHECK-DAG:     [[SIGN2:%.*]] = extractelement <3 x half> %sign, i32 2
; CHECK-DAG:     [[MAG0:%.*]] = extractelement <3 x half> %mag, i32 0
; CHECK-DAG:     [[MAG2:%.*]] = extractelement <3 x half> %mag, i32 2
; CHECK:         call half @llvm.copysign.f16(half [[MAG0]], half [[SIGN0]])
; CHECK:         call half @llvm.copysign.f16(half %{{.*}}, half %{{.*}})
; CHECK:         call half @llvm.copysign.f16(half [[MAG2]], half [[SIGN2]])
; CHECK-NOT:     call <3 x half> @llvm.copysign
  %r = call <3 x half> @llvm.copysign.v3f16(<3 x half> %mag, <3 x half> %sign)
  ret <3 x half> %r
}

define <4 x float> @test_copysign_v4f32_const_sign(<4 x float> %mag) {
; CHECK-LABEL: @test_copysign_v4f32_const_sign(
; CHECK-COUNT-4: call float @llvm.copysign.f32(float %{{.*}}, float -1.000000e+00)
; CHECK-NOT:     call <4 x float> @llvm.copysign
  %r = call <4 x float> @llvm.copysign.v4f32(<4 x float> %mag, <4 x float> <float -1.0, float -1.0, float -1.0, float -1.0>)
  ret <4 x float> %r
}

; Fast-math flags carry over to the scalar calls.
define <2 x float> @test_copysign_v2f32_fmf(<2 x float> %mag, <2 x float> %sign) {
; CHECK-LABEL: @test_copysign_v2f32_fmf(
; CHECK-COUNT-2: call nnan nsz float @llvm.copysign.f32(
; CHECK-NOT:     call {{.*}}<2 x float> @llvm.copysign
  %r = call nnan nsz <2 x float> @llvm.copysign.v2f32(<2 x float> %mag, <2 x float> %sign)
  ret <2 x float> %r
}

declare <3 x half> @llvm.copysign.v3f16(<3 x half>, <3 x half>)
declare <2 x float> @llvm.copysign.v2f32(<2 x float>, <2 x float>)
declare <4 x float> @llvm.copysign.v4f32(<4 x float>, <4 x float>)
