;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <64 x i32> @llvm.vc.internal.round.to.tf32.v64i32.v64f32(<64 x float>)

define <64 x i32> @test_out(<64 x float> %src) {
  ; CHECK-DAG: %[[SPLIT0:.+]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %src, i32 32, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK-DAG: %[[RES0:.+]] = call <32 x i32> @llvm.vc.internal.round.to.tf32.v32i32.v32f32(<32 x float> %[[SPLIT0]])
  ; CHECK-DAG: %[[SPLIT32:.+]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %src, i32 32, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK-DAG: %[[RES32:.+]] = call <32 x i32> @llvm.vc.internal.round.to.tf32.v32i32.v32f32(<32 x float> %[[SPLIT32]])
  %res = call <64 x i32> @llvm.vc.internal.round.to.tf32.v64i32.v64f32(<64 x float> %src)
  ret <64 x i32> %res
}
