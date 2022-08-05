;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests strengthReducePowOrExpLog

declare float @llvm.pow.f32(float, float)

; x^(1/2) = sqrt(x)
define float @test1(float %x) #0 {
entry:
  %0 = call float @llvm.pow.f32(float %x, float 5.000000e-01)
  ret float %0
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: llvm.pow.f32
; CHECK: %0 = call float @llvm.sqrt.f32(float %x)
; CHECK: ret float %0

; x^1 = x
define float @test2(float %x) #0 {
entry:
  %0 = call float @llvm.pow.f32(float %x, float 1.000000e+00)
  ret float %0
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: llvm.pow.f32
; CHECK: ret float %x

; x^2 = x*x
define float @test3(float %x) #0 {
entry:
  %0 = call float @llvm.pow.f32(float %x, float 2.000000e+00)
  ret float %0
}

; CHECK-LABEL: define float @test3
; CHECK-NOT: llvm.pow.f32
; CHECK: %0 = fmul float %x, %x
; CHECK: ret float %0

; x^3 = x*x*x
define float @test4(float %x) #0 {
entry:
  %0 = call float @llvm.pow.f32(float %x, float 3.000000e+00)
  ret float %0
}

; CHECK-LABEL: define float @test4
; CHECK-NOT: llvm.pow.f32
; CHECK: %0 = fmul float %x, %x
; CHECK: %1 = fmul float %0, %x
; CHECK: ret float %1

; x^4 = (x*x) * (x*x)
define float @test5(float %x) #0 {
entry:
  %0 = call float @llvm.pow.f32(float %x, float 4.000000e+00)
  ret float %0
}

; CHECK-LABEL: define float @test5
; CHECK-NOT: llvm.pow.f32
; CHECK: %0 = fmul float %x, %x
; CHECK: %1 = fmul float %0, %0
; CHECK: ret float %1

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
