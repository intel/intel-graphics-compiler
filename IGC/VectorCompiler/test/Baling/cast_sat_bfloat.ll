;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x float> @llvm.genx.sat.v16f32(<16 x float>)

define <16 x float> @test_ext(<16 x bfloat> %src) {
; CHECK: bales in function: test_ext:
; CHECK-NEXT:   %sat = call <16 x float> @llvm.genx.sat.v16f32(<16 x float> %ext):
; CHECK-SAME: {{ saturate$}}
; CHECK-NEXT: GenXBaling dump end
  %ext = fpext <16 x bfloat> %src to <16 x float>
  %sat = call <16 x float> @llvm.genx.sat.v16f32(<16 x float> %ext)
  ret <16 x float> %sat
}
