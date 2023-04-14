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
  %0 = fadd float %x, 2.000000e+00
  %1 = fdiv float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK: %0 = fdiv float 1.000000e+00, 2.000000e+00
; CHECK: %1 = fmul float %x, %0
; CHECK: %2 = fadd float %1, 1.000000e+00
; CHECK: ret float %2

; (x - 2) / 2 => (x * 1/2) - 1
define float @test2(float %x) #0 {
entry:
  %0 = fsub float %x, 2.000000e+00
  %1 = fdiv float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test2
; CHECK: %0 = fdiv float 1.000000e+00, 2.000000e+00
; CHECK: %1 = fmul float %x, %0
; CHECK: %2 = fsub float %1, 1.000000e+00
; CHECK: ret float %2

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
