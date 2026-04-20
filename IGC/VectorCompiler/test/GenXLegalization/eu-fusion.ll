;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: This test checks that we do more strict legalization for platforms with fused EUs.

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP -S < %s | FileCheck %s --check-prefix=CHECK-XeLP
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -S < %s | FileCheck %s --check-prefix=CHECK-XeHPG
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLPG -S < %s | FileCheck %s --check-prefix=CHECK-XeLPG
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLPGPlus -S < %s | FileCheck %s --check-prefix=CHECK-XeLPG
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefix=CHECK-XeHPC
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe2 -S < %s | FileCheck %s --check-prefix=CHECK-Xe2


; CHECK-XeHPC: add <32 x i16>
; CHECK-Xe2: add <32 x i16>

; CHECK-XeLP: add <16 x i16>
; CHECK-XeHPG: add <16 x i16>
; CHECK-XeLPG: add <16 x i16>
; CHECK-XeLPG: add <16 x i16>

define <32 x i16> @test(<32 x i16> %a) #1 {
entry:
  %res = add <32 x i16> %a, %a
  ret <32 x i16> %res
}

attributes #1 = { "CMStackCall" }
