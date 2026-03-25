;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorAddSubOp

; (1 - x) + x = 1
define float @test1(float %x) #0 {
entry:
  %0 = fsub float 1.000000e+00, %x
  %1 = fadd float %0, %x
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fsub
; CHECK-NOT: fadd
; CHECK: ret float 1.000000e+00

; x - (x - 1) = 1
define float @test2(float %x) #0 {
entry:
  %0 = fsub float %x, 1.000000e+00
  %1 = fsub float %x, %0
  ret float %1
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fsub
; CHECK: ret float 1.000000e+00

; (1 + x) - x = 1
define float @test3(float %x) #0 {
entry:
  %0 = fadd float 1.000000e+00, %x
  %1 = fsub float %0, %x
  ret float %1
}

; CHECK-LABEL: define float @test3
; CHECK-NOT: fsub
; CHECK-NOT: fadd
; CHECK: ret float 1.000000e+00

; x - (1 + x) = -1
define float @test4(float %x) #0 {
entry:
  %0 = fadd float 1.000000e+00, %x
  %1 = fsub float %x, %0
  ret float %1
}

; CHECK-LABEL: define float @test4
; CHECK-NOT: fadd
; CHECK: %0 = fsub float 0.000000e+00, 1.000000e+00
; CHECK: ret float %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
