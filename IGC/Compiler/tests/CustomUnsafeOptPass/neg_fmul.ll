;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorNegateMultiply

; 0 - (x * 2) => -x * 2
define float @test1(float %x) #0 {
entry:
  %0 = fmul fast float %x, 2.000000e+00
  %1 = fsub fast float 0.000000e+00, %0
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK: %0 = fsub fast float 0.000000e+00, %x
; CHECK: %1 = fmul fast float %0, 2.000000e+00
; CHECK-NOT: fsub{{.*}}float 0.000000e+00, {{%[0-9]+}}
; CHECK: ret float %1
