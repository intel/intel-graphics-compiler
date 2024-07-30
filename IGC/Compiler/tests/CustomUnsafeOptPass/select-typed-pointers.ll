;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitSelectInst

; y = a*x + b
; x == 0 ? b : y
; but if x == 0, then y == b, so always return y
define float @test1(float %x, float %a, float %b) #0 {
entry:
  %0 = fmul float %a, %x
  %1 = fadd float %0, %b
  %2 = fcmp oeq float %x, 0.000000e+00
  %3 = select i1 %2, float %b, float %1
  ret float %3
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fcmp
; CHECK-NOT: select
; CHECK: ret float %1

; y = (a - b)*x + b
; y == 1 ? a : y
; but if x == 1, then y == a, so always return y
define float @test2(float %x, float %a, float %b) #0 {
entry:
  %0 = fsub float %a, %b
  %1 = fmul float %0, %x
  %2 = fadd float %1, %b
  %3 = fcmp oeq float %x, 1.000000e+00
  %4 = select i1 %3, float %a, float %2
  ret float %4
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fcmp
; CHECK-NOT: select
; CHECK: ret float %2

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
