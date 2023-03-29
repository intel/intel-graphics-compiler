;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorDivDivOp

; 1 / (1 / x) = x
define float @test1(float %x) #0 {
entry:
  %0 = fdiv fast float 1.000000e+00, %x
  %1 = fdiv fast float 1.000000e+00, %0
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fdiv
; CHECK: ret float %x

; 4 / (1 / x) = 4 * x
define float @test2(float %x) #0 {
entry:
  %0 = fdiv fast float 1.000000e+00, %x
  %1 = fdiv fast float 4.000000e+00, %0
  ret float %1
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fdiv
; CHECK: %0 = fmul fast float 4.000000e+00, %x
; CHECK: ret float %0
