;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorTwoConstants

; (x + 1) + 2 => x + 3
define float @test1(float %x) #0 {
entry:
  %0 = fadd float %x, 1.000000e+00
  %1 = fadd float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: 1.000000e+00
; CHECK: %0 = fadd float %x, 3.000000e+00
; CHECK: ret float %0

; 3 - (2 - x) => 1 + x
define float @test2(float %x) #0 {
entry:
  %0 = fsub float 2.000000e+00, %x
  %1 = fsub float 3.000000e+00, %0
  ret float %1
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: 2.000000e+00
; CHECK: %0 = fadd float %x, 1.000000e+00
; CHECK: ret float %0

; (2 - x) + 1 => 3 - x
define float @test3(float %x) #0 {
entry:
  %0 = fsub float 2.000000e+00, %x
  %1 = fadd float %0, 1.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test3
; CHECK-NOT: 2.000000e+00
; CHECK: %0 = fsub float 3.000000e+00, %x
; CHECK: ret float %0

; (x + 2) + 2 => x
define float @test4(float %x) #0 {
entry:
  %0 = fadd float %x, 2.000000e+00
  %1 = fsub float %0, 2.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test4
; CHECK-NOT: fadd
; CHECK-NOT: fsub
; CHECK: ret float %x

; (x * 2) * 3 => x * 6
define float @test5(float %x) #0 {
entry:
  %0 = fmul float %x, 2.000000e+00
  %1 = fmul float %0, 3.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @test5
; CHECK-NOT: 2.000000e+00
; CHECK: %0 = fmul float %x, 6.000000e+00
; CHECK: ret float %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
