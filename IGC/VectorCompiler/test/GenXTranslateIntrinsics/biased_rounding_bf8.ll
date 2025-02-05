;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3 -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3 -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <32 x i8> @llvm.genx.biased.rounding.bf8.v32i8.v32f16(<32 x half>, <32 x i8>)

; CHECK-LABEL: @test_v32f16
define <32 x i8> @test_v32f16(<32 x half> %src, <32 x i8> %bias) {
; CHECK: %res = call <32 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v32i8.v32f16.v32i8(<32 x half> %src, <32 x i8> %bias)
  %res = call <32 x i8> @llvm.genx.biased.rounding.bf8.v32i8.v32f16(<32 x half> %src, <32 x i8> %bias)
  ret <32 x i8> %res
}
