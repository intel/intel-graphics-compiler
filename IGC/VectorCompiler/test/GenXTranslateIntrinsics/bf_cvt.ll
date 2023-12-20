;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x float> @llvm.genx.bf.cvt.v32f32.v32f16(<32 x half>)
declare <32 x half> @llvm.genx.bf.cvt.v32f16.v32f32(<32 x float>)

define <32 x float> @from(<32 x half> %arg) {
  ; CHECK: [[BITCAST:%[^ ]+]] = bitcast <32 x half> %arg to <32 x bfloat>
  ; CHECK: %res = fpext <32 x bfloat> [[BITCAST]] to <32 x float>
  %res = call <32 x float> @llvm.genx.bf.cvt.v32f32.v32f16(<32 x half> %arg)
  ret <32 x float> %res
}

define <32 x half> @to(<32 x float> %arg) {
  ; CHECK: [[CVT:%[^ ]+]] = fptrunc <32 x float> %arg to <32 x bfloat>
  ; CHECK: %res = bitcast <32 x bfloat> [[CVT]] to <32 x half>
  %res = call <32 x half> @llvm.genx.bf.cvt.v32f16.v32f32(<32 x float> %arg)
  ret <32 x half> %res
}
