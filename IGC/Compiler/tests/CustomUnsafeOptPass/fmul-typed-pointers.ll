;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; 0 * x = 0
define float @test1(float %x) #0 {
entry:
  %0 = fmul float 0.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fmul
; CHECK: ret float 0.000000e+00

; 1 * x = x
define float @test2(float %x) #0 {
entry:
  %0 = fmul float 1.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fmul
; CHECK: ret float %x

; x * 1 = x
define float @test3(float %x) #0 {
entry:
  %0 = fmul float %x, 1.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @test3
; CHECK-NOT: fmul
; CHECK: ret float %x

; -1 * x = -x
define float @test4(float %x) #0 {
entry:
  %0 = fmul float -1.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @test4
; CHECK-NOT: fmul
; CHECK: %0 = fsub float 0.000000e+00, %x
; CHECK: ret float %0

; x * -1 = -x
define float @test5(float %x) #0 {
entry:
  %0 = fmul float %x, -1.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @test5
; CHECK-NOT: fmul
; CHECK: %0 = fsub float 0.000000e+00, %x
; CHECK: ret float %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
