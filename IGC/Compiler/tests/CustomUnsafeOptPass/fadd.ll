;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; 0 + x = x
define float @test1(float %x) #0 {
entry:
  %0 = fadd float 0.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fadd
; CHECK: ret float %x

; x + 0 = x
define float @test2(float %x) #0 {
entry:
  %0 = fadd float %x, 0.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fadd
; CHECK: ret float %x

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
