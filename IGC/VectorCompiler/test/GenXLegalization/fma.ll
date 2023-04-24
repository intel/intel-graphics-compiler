;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-WIDE %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <32 x float> @llvm.fma.v32f32(<32 x float>, <32 x float>, <32 x float>)

define <32 x float> @test(<32 x float> %a, <32 x float> %b, <32 x float> %c) {
  ; CHECK: call <16 x float> @llvm.fma.v16f32(<16 x float> %{{[^,]+}}, <16 x float> %{{[^,]+}}, <16 x float> %{{[^)]+}})
  ; CHECK: call <16 x float> @llvm.fma.v16f32(<16 x float> %{{[^,]+}}, <16 x float> %{{[^,]+}}, <16 x float> %{{[^)]+}})
  ; CHECK-WIDE: %r = call <32 x float> @llvm.fma.v32f32(<32 x float> %a, <32 x float> %b, <32 x float> %c)
  %r = call <32 x float> @llvm.fma.v32f32(<32 x float> %a, <32 x float> %b, <32 x float> %c)
  ret <32 x float> %r
}
