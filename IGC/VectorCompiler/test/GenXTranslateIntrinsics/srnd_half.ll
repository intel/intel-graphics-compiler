;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x half> @llvm.genx.srnd.v32f16.v32f32.v32f32(<32 x float>, <32 x float>)
declare <32 x half> @llvm.genx.srnd.v32f16.v32f32.v32i16(<32 x float>, <32 x i16>)

define <32 x half> @test_1(<32 x float> %arg, <32 x float> %rnd) {
; CHECK: [[BC:%[^ ]+]] = bitcast <32 x float> %rnd to <32 x i32>
; CHECK: [[TRUNC:%[^ ]+]] = trunc <32 x i32> [[BC]] to <32 x i16>
; CHECK: %res = call <32 x half> @llvm.vc.internal.stochastic.round.to.f16.v32f16.v32f32.v32i16(<32 x float> %arg, <32 x i16> [[TRUNC]])
  %res = call <32 x half> @llvm.genx.srnd.v32f16.v32f32.v32f32(<32 x float> %arg, <32 x float> %rnd)
  ret <32 x half> %res
}

define <32 x half> @test_2(<32 x float> %arg, <32 x i16> %rnd) {
; CHECK: %res = call <32 x half> @llvm.vc.internal.stochastic.round.to.f16.v32f16.v32f32.v32i16(<32 x float> %arg, <32 x i16> %rnd)
  %res = call <32 x half> @llvm.genx.srnd.v32f16.v32f32.v32i16(<32 x float> %arg, <32 x i16> %rnd)
  ret <32 x half> %res
}
