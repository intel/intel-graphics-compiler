;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.tf32.cvt.v32i32.v32f32(<32 x float>)

define <32 x i32> @to(<32 x float> %arg) {
  ; CHECK: %res = call <32 x i32> @llvm.vc.internal.round.to.tf32.v32i32.v32f32(<32 x float> %arg)
  %res = call <32 x i32> @llvm.genx.tf32.cvt.v32i32.v32f32(<32 x float> %arg)
  ret <32 x i32> %res
}
