;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <4 x half> @llvm.copysign.v4f16(<4 x half>, <4 x half>)
declare <4 x bfloat> @llvm.copysign.v4bf16(<4 x bfloat>, <4 x bfloat>)
declare <4 x float> @llvm.copysign.v4f32(<4 x float>, <4 x float>)
declare <4 x double> @llvm.copysign.v4f64(<4 x double>, <4 x double>)

; CHECK-LABEL: @test_v4f16
define <4 x half> @test_v4f16(<4 x half> %src, <4 x half> %sign) {
; CHECK: [[MAG:%.*]] = bitcast <4 x half> %src to <4 x i16>
; CHECK: [[SGN:%.*]] = bitcast <4 x half> %sign to <4 x i16>
; CHECK: [[ABS:%.*]] = and <4 x i16> [[MAG]], <i16 32767, i16 32767, i16 32767, i16 32767>
; CHECK: [[SIGN:%.*]] = and <4 x i16> [[SGN]], <i16 -32768, i16 -32768, i16 -32768, i16 -32768>
; CHECK: [[RES:%.*]] = or <4 x i16> [[ABS]], [[SIGN]]
; CHECK: [[RES_HALF:%.*]] = bitcast <4 x i16> [[RES]] to <4 x half>
; CHECK: ret <4 x half> [[RES_HALF]]
  %res = call <4 x half> @llvm.copysign.v4f16(<4 x half> %src, <4 x half> %sign)
  ret <4 x half> %res
}

; CHECK-LABEL: @test_v4bf16
define <4 x bfloat> @test_v4bf16(<4 x bfloat> %src, <4 x bfloat> %sign) {
; CHECK: [[MAG:%.*]] = bitcast <4 x bfloat> %src to <4 x i16>
; CHECK: [[SGN:%.*]] = bitcast <4 x bfloat> %sign to <4 x i16>
; CHECK: [[ABS:%.*]] = and <4 x i16> [[MAG]], <i16 32767, i16 32767, i16 32767, i16 32767>
; CHECK: [[SIGN:%.*]] = and <4 x i16> [[SGN]], <i16 -32768, i16 -32768, i16 -32768, i16 -32768>
; CHECK: [[RES:%.*]] = or <4 x i16> [[ABS]], [[SIGN]]
; CHECK: [[RES_BF:%.*]] = bitcast <4 x i16> [[RES]] to <4 x bfloat>
; CHECK: ret <4 x bfloat> [[RES_BF]]
  %res = call <4 x bfloat> @llvm.copysign.v4bf16(<4 x bfloat> %src, <4 x bfloat> %sign)
  ret <4 x bfloat> %res
}

; CHECK-LABEL: @test_v4f32
define <4 x float> @test_v4f32(<4 x float> %src, <4 x float> %sign) {
; CHECK: [[MAG:%.*]] = bitcast <4 x float> %src to <8 x i16>
; CHECK: [[SGN:%.*]] = bitcast <4 x float> %sign to <8 x i16>
; CHECK: [[MAG_EXTRACT:%.*]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v8i16.i16(<8 x i16> [[MAG]], i32 2, i32 1, i32 0, i16 2, i32 undef)
; CHECK: [[SGN_EXTRACT:%.*]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v8i16.i16(<8 x i16> [[SGN]], i32 2, i32 1, i32 0, i16 2, i32 undef)
; CHECK: [[ABS:%.*]] = and <4 x i16> [[MAG_EXTRACT]], <i16 32767, i16 32767, i16 32767, i16 32767>
; CHECK: [[SIGN:%.*]] = and <4 x i16> [[SGN_EXTRACT]], <i16 -32768, i16 -32768, i16 -32768, i16 -32768>
; CHECK: [[RES:%.*]] = or <4 x i16> [[ABS]], [[SIGN]]
; CHECK: [[RES_INSERT:%.*]] = call <8 x i16> @llvm.genx.wrregioni.v8i16.v4i16.i16.i1(<8 x i16> [[MAG]], <4 x i16> [[RES]], i32 2, i32 1, i32 0, i16 2, i32 undef, i1 true)
; CHECK: [[RES_FLOAT:%.*]] = bitcast <8 x i16> [[RES_INSERT]] to <4 x float>
; CHECK: ret <4 x float> [[RES_FLOAT]]
  %res = call <4 x float> @llvm.copysign.v4f32(<4 x float> %src, <4 x float> %sign)
  ret <4 x float> %res
}

; CHECK-LABEL: @test_v4f64
define <4 x double> @test_v4f64(<4 x double> %src, <4 x double> %sign) {
; CHECK: [[MAG:%.*]] = bitcast <4 x double> %src to <16 x i16>
; CHECK: [[SGN:%.*]] = bitcast <4 x double> %sign to <16 x i16>
; CHECK: [[MAG_EXTRACT:%.*]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16.i16(<16 x i16> [[MAG]], i32 4, i32 1, i32 0, i16 6, i32 undef)
; CHECK: [[SGN_EXTRACT:%.*]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16.i16(<16 x i16> [[SGN]], i32 4, i32 1, i32 0, i16 6, i32 undef)
; CHECK: [[ABS:%.*]] = and <4 x i16> [[MAG_EXTRACT]], <i16 32767, i16 32767, i16 32767, i16 32767>
; CHECK: [[SIGN:%.*]] = and <4 x i16> [[SGN_EXTRACT]], <i16 -32768, i16 -32768, i16 -32768, i16 -32768>
; CHECK: [[RES:%.*]] = or <4 x i16> [[ABS]], [[SIGN]]
; CHECK: [[RES_INSERT:%.*]] = call <16 x i16> @llvm.genx.wrregioni.v16i16.v4i16.i16.i1(<16 x i16> [[MAG]], <4 x i16> [[RES]], i32 4, i32 1, i32 0, i16 6, i32 undef, i1 true)
; CHECK: [[RES_DOUBLE:%.*]] = bitcast <16 x i16> [[RES_INSERT]] to <4 x double>
; CHECK: ret <4 x double> [[RES_DOUBLE]]
  %res = call <4 x double> @llvm.copysign.v4f64(<4 x double> %src, <4 x double> %sign)
  ret <4 x double> %res
}
