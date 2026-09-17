;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey ClampNegativeMipLODBiasToZero=1 --ClampNegativeMipLODBiasToZero -S %s | FileCheck %s

; CHECK-LABEL: define void @test_sampleL
; CHECK-NOT: llvm.maxnum
; CHECK: call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(
; CHECK-SAME: float %lod,
; CHECK-SAME: float %u,
; CHECK-SAME: float %v,
; CHECK-NOT: llvm.maxnum
; CHECK: ret void
define void @test_sampleL(float %lod, float %u, float %v) {
entry:
  %color = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p0.p0.p0(
      float %lod,
      float %u,
      float %v,
      float 0.000000e+00,
      float poison,
      i8* null,
      i8* null,
      i8* null,
      i32 0, i32 0, i32 0)
  ret void
}

; CHECK-LABEL: define void @test_samplePOB_variable
; CHECK-NOT: llvm.maxnum
; CHECK: call <4 x float> @llvm.genx.GenISA.samplePOBptr{{.*}}(
; CHECK-SAME: float %bias,
; CHECK-SAME: float %u,
; CHECK-SAME: float %v,
; CHECK-NOT: llvm.maxnum
; CHECK: ret void
define void @test_samplePOB_variable(float %bias, float %u, float %v) {
entry:
  %color = call <4 x float> @llvm.genx.GenISA.samplePOBptr.v4f32.f32.p0.p0.p0(
      float %bias,
      float %u,
      float %v,
      float 0.000000e+00,
      float poison,
      i8* null,
      i8* null,
      i8* null,
      i32 0, i32 0, i32 0)
  ret void
}

; CHECK-LABEL: define void @test_samplePOB_const_negative
; CHECK: call <4 x float> @llvm.genx.GenISA.samplePOBptr{{.*}}(
; CHECK-SAME: float -2.000000e+00,
; CHECK-SAME: float %u,
; CHECK-SAME: float %v,
; CHECK-NOT: llvm.maxnum
; CHECK: ret void
define void @test_samplePOB_const_negative(float %u, float %v) {
entry:
  %color = call <4 x float> @llvm.genx.GenISA.samplePOBptr.v4f32.f32.p0.p0.p0(
      float -2.000000e+00,
      float %u,
      float %v,
      float 0.000000e+00,
      float poison,
      i8* null,
      i8* null,
      i8* null,
      i32 0, i32 0, i32 0)
  ret void
}

; CHECK-LABEL: define void @test_sampleBCMlod_variable
; CHECK-NOT: llvm.maxnum
; CHECK: call <4 x float> @llvm.genx.GenISA.sampleBCMlodptr{{.*}}(
; CHECK-SAME: float %ref,
; CHECK-SAME: float %bias,
; CHECK-SAME: float %u,
; CHECK-SAME: float %v,
; CHECK-NOT: llvm.maxnum
; CHECK: ret void
define void @test_sampleBCMlod_variable(float %ref, float %bias, float %u, float %v) {
entry:
  %color = call <4 x float> @llvm.genx.GenISA.sampleBCMlodptr.v4f32.f32.p0.p0.p0(
      float %ref,
      float %bias,
      float %u,
      float %v,
      float 0.000000e+00,
      float poison,
      float 0.000000e+00,
      i8* null,
      i8* null,
      i8* null,
      i32 0, i32 0, i32 0)
  ret void
}

; CHECK-LABEL: define void @test_sampleBCMlod_const_negative
; CHECK: call <4 x float> @llvm.genx.GenISA.sampleBCMlodptr{{.*}}(
; CHECK-SAME: float %ref,
; CHECK-SAME: float -2.000000e+00,
; CHECK-SAME: float %u,
; CHECK-SAME: float %v,
; CHECK-NOT: llvm.maxnum
; CHECK: ret void
define void @test_sampleBCMlod_const_negative(float %ref, float %u, float %v) {
entry:
  %color = call <4 x float> @llvm.genx.GenISA.sampleBCMlodptr.v4f32.f32.p0.p0.p0(
      float %ref,
      float -2.000000e+00,
      float %u,
      float %v,
      float 0.000000e+00,
      float poison,
      float 0.000000e+00,
      i8* null,
      i8* null,
      i8* null,
      i32 0, i32 0, i32 0)
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p0.p0.p0(float, float, float, float, float, i8*, i8*, i8*, i32, i32, i32)
declare <4 x float> @llvm.genx.GenISA.samplePOBptr.v4f32.f32.p0.p0.p0(float, float, float, float, float, i8*, i8*, i8*, i32, i32, i32)
declare <4 x float> @llvm.genx.GenISA.sampleBCMlodptr.v4f32.f32.p0.p0.p0(float, float, float, float, float, float, float, i8*, i8*, i8*, i32, i32, i32)
