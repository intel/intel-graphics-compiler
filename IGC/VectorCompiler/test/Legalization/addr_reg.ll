;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32>, i32, i32, i32, <32 x i16>, i32)

; CHECK-LABEL: test
define <32 x i32> @test(<32 x i32> %arg1, <32 x i16> %arg2) {
  ; CHECK-NEXT: tail call <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32> %arg1, i32 0, i32 1, i32 0, <32 x i16> %arg2, i32 undef)
  %ins = tail call <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32> %arg1, i32 0, i32 1, i32 0, <32 x i16> %arg2, i32 undef)
  ret <32 x i32> %ins
}
