;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.vc.internal.optimization.fence.v16i32(<16 x i32>)

; CHECK-LABEL: @test(
define <16 x i32> @test(<16 x i32> %a) {
; CHECK: ret <16 x i32> %a
  %call = call <16 x i32> @llvm.vc.internal.optimization.fence.v16i32(<16 x i32> %a)
  ret <16 x i32> %call
}
