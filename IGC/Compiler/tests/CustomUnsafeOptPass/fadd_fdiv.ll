;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorAddDiv

; (x + 2) / 2 => (x * 1/2) + 1
define float @test1(float %x) #0 {
entry:
  %0 = fadd fast float %x, 2.000000e+00
  %1 = fdiv fast float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK: %0 = fdiv fast float 1.000000e+00, 2.000000e+00
; CHECK: %1 = fmul fast float %x, %0
; CHECK: %2 = fadd fast float %1, 1.000000e+00
; CHECK: ret float %2

; (x - 2) / 2 => (x * 1/2) - 1
define float @test2(float %x) #0 {
entry:
  %0 = fsub fast float %x, 2.000000e+00
  %1 = fdiv fast float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test2
; CHECK: %0 = fdiv fast float 1.000000e+00, 2.000000e+00
; CHECK: %1 = fmul fast float %x, %0
; CHECK: %2 = fsub fast float %1, 1.000000e+00
; CHECK: ret float %2
