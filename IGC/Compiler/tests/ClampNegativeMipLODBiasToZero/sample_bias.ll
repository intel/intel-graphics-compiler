;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey ClampNegativeMipLODBiasToZero=1 --ClampNegativeMipLODBiasToZero -S %s | FileCheck %s
; CHECK-LABEL: define void @test_variable_bias
define void @test_variable_bias(float %bias, float %u, float %v) {
entry:
  ; CHECK: [[CLAMPED:%[a-zA-Z0-9_.]+]] = call float @llvm.maxnum.f32(float %bias, float 0.000000e+00)
  ; CHECK-NEXT: call <4 x float> @llvm.genx.GenISA.sampleBptr{{.*}}(
  ; CHECK-SAME: float [[CLAMPED]],
  ; CHECK-SAME: float %u,
  ; CHECK-SAME: float %v,
  %color = call <4 x float> @llvm.genx.GenISA.sampleBptr.v4f32.f32.p0.p0.p0(
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

declare <4 x float> @llvm.genx.GenISA.sampleBptr.v4f32.f32.p0.p0.p0(float, float, float, float, float, float, i8*, i8*, i8*, i32, i32, i32)
