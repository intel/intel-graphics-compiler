;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32>, i32, i32, i32, <32 x i16>, i32)

; CHECK-LABEL: test
define <32 x i32> @test(<32 x i32> %arg1, <32 x i16> %arg2) {
  ; CHECK: [[SPLIT0_IND:%[^ ]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %arg2, i32 16, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[SPLIT0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.v16i16(<32 x i32> %arg1, i32 0, i32 1, i32 0, <16 x i16> [[SPLIT0_IND]], i32 0)
  ; CHECK: [[SPLIT0_JOIN:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[SPLIT0]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[SPLIT16_IND:%[^ ]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %arg2, i32 16, i32 16, i32 1, i16 32, i32 undef)
  ; CHECK: [[SPLIT16:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.v16i16(<32 x i32> %arg1, i32 0, i32 1, i32 0, <16 x i16> [[SPLIT16_IND]], i32 0)
  ; CHECK: call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[SPLIT0_JOIN]], <16 x i32> [[SPLIT16]], i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ins = tail call <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32> %arg1, i32 0, i32 1, i32 0, <32 x i16> %arg2, i32 undef)
  ret <32 x i32> %ins
}
