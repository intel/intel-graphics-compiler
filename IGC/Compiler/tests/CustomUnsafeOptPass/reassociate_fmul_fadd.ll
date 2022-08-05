;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests reassociateMulAdd

; 2*x + 3*y + 4 => 2*x + 4 + 3*y
define float @test1(float %x, float %y) #0 {
entry:
  %0 = fmul float 2.000000e+00, %x
  %1 = fmul float 3.000000e+00, %y
  %2 = fadd float %0, %1
  %3 = fadd float %2, 4.000000e+00
  ret float %3
}

; CHECK-LABEL: define float @test1
; CHECK: %0 = fmul float 2.000000e+00, %x
; CHECK: %1 = fmul float 3.000000e+00, %y
; CHECK: %2 = fadd float %0, 4.000000e+00
; CHECK: %3 = fadd float %2, %1
; CHECK: ret float %3

; 2*x + 3*y + (x + y) => 2*x + (x + y) + 3*y
define float @test2(float %x, float %y) #0 {
entry:
  %0 = fmul float 2.000000e+00, %x
  %1 = fmul float 3.000000e+00, %y
  %2 = fadd float %0, %1
  %3 = fadd float %x, %y
  %4 = fadd float %2, %3
  ret float %4
}

; CHECK-LABEL: define float @test2
; CHECK: %0 = fmul float 2.000000e+00, %x
; CHECK: %1 = fmul float 3.000000e+00, %y
; CHECK: %2 = fadd float %x, %y
; CHECK: %3 = fadd float %0, %2
; CHECK: %4 = fadd float %3, %1
; CHECK: ret float %4

; 2*x + 3*y + 4*z - 5 => 2*x - 5 + 4*z + 3*y
define float @test3(float %x, float %y, float %z) #0 {
entry:
  %0 = fmul float 2.000000e+00, %x
  %1 = fmul float 3.000000e+00, %y
  %2 = fmul float 4.000000e+00, %z
  %3 = fadd float %0, %1
  %4 = fadd float %3, %2
  %5 = fsub float %4, 5.000000e+00
  ret float %5
}

; CHECK-LABEL: define float @test3
; CHECK: %0 = fmul float 2.000000e+00, %x
; CHECK: %1 = fmul float 3.000000e+00, %y
; CHECK: %2 = fmul float 4.000000e+00, %z
; CHECK: %3 = fsub float %0, 5.000000e+00
; CHECK: %4 = fadd float %3, %2
; CHECK: %5 = fadd float %4, %1
; CHECK: ret float %5

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"MadEnable", i1 true}
