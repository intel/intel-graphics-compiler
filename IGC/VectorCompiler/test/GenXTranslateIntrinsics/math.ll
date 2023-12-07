;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x float> @llvm.genx.cos.v32f32(<32 x float>)
declare <32 x float> @llvm.genx.exp.v32f32(<32 x float>)
declare <32 x float> @llvm.genx.log.v32f32(<32 x float>)
declare <32 x float> @llvm.genx.sin.v32f32(<32 x float>)
declare <32 x float> @llvm.genx.pow.v32f32(<32 x float>, <32 x float>)

; CHECK-LABEL: test_cos
define <32 x float> @test_cos(<32 x float> %arg) {
  ; CHECK: call afn <32 x float> @llvm.cos.v32f32(<32 x float> %arg)
  %res = call <32 x float> @llvm.genx.cos.v32f32(<32 x float> %arg)
  ret <32 x float> %res
}

; CHECK-LABEL: test_exp
define <32 x float> @test_exp(<32 x float> %arg) {
  ; CHECK: call afn <32 x float> @llvm.exp2.v32f32(<32 x float> %arg)
  %res = call <32 x float> @llvm.genx.exp.v32f32(<32 x float> %arg)
  ret <32 x float> %res
}

; CHECK-LABEL: test_log
define <32 x float> @test_log(<32 x float> %arg) {
  ; CHECK: call afn <32 x float> @llvm.log2.v32f32(<32 x float> %arg)
  %res = call <32 x float> @llvm.genx.log.v32f32(<32 x float> %arg)
  ret <32 x float> %res
}

; CHECK-LABEL: test_sin
define <32 x float> @test_sin(<32 x float> %arg) {
  ; CHECK: call afn <32 x float> @llvm.sin.v32f32(<32 x float> %arg)
  %res = call <32 x float> @llvm.genx.sin.v32f32(<32 x float> %arg)
  ret <32 x float> %res
}

; CHECK-LABEL: test_pow
define <32 x float> @test_pow(<32 x float> %a, <32 x float> %b) {
  ; CHECK: call afn <32 x float> @llvm.pow.v32f32(<32 x float> %a, <32 x float> %b)
  %res = call <32 x float> @llvm.genx.pow.v32f32(<32 x float> %a, <32 x float> %b)
  ret <32 x float> %res
}
