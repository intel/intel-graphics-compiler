;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitBinaryOperatorFmulFaddPropagation

define <3 x float> @test1(float %a, float %x, float %y, float %z) #0 {
entry:
  %0 = fadd float %a, %x
  %1 = fadd float %a, %y
  %2 = fadd float %a, %z
  %3 = fadd float %0, 2.000000e+00 ; a + x + 2
  %4 = fadd float %1, 2.000000e+00 ; a + y + 2
  %5 = fadd float %2, 2.000000e+00 ; a + z + 2 => a + 2 is common, calculate once
  %6 = insertelement <3 x float> undef, float %3, i64 0
  %7 = insertelement <3 x float> %6,    float %4, i64 1
  %8 = insertelement <3 x float> %7,    float %5, i64 2
  ret <3 x float> %8
}

; CHECK-LABEL: define <3 x float> @test1
; CHECK-NOT: fadd float %a, %x
; CHECK-NOT: fadd float %a, %y
; CHECK-NOT: fadd float %a, %z
; CHECK: %0 = fadd float %a, 2.000000e+00
; CHECK: %1 = fadd float %0, %x
; CHECK: %2 = fadd float %0, %y
; CHECK: %3 = fadd float %0, %z

define <3 x float> @test2(float %a, float %x, float %y, float %z) #0 {
entry:
  %0 = fmul float %a, %x
  %1 = fmul float %a, %y
  %2 = fmul float %a, %z
  %3 = fmul float %0, 2.000000e+00 ; a * x * 2
  %4 = fmul float %1, 2.000000e+00 ; a * y * 2
  %5 = fmul float %2, 2.000000e+00 ; a * z * 2 => a * 2 is common, calculate once
  %6 = insertelement <3 x float> undef, float %3, i64 0
  %7 = insertelement <3 x float> %6,    float %4, i64 1
  %8 = insertelement <3 x float> %7,    float %5, i64 2
  ret <3 x float> %8
}

; CHECK-LABEL: define <3 x float> @test2
; CHECK-NOT: fmul float %a, %x
; CHECK-NOT: fmul float %a, %y
; CHECK-NOT: fmul float %a, %z
; CHECK: %0 = fmul float %a, 2.000000e+00
; CHECK: %1 = fmul float %0, %x
; CHECK: %2 = fmul float %0, %y
; CHECK: %3 = fmul float %0, %z

define <3 x float> @test3(float %a, float %x, float %y, float %z) #0 {
entry:
  %0 = fadd float %x, %a
  %1 = fadd float %y, %a
  %2 = fadd float %z, %a
  %3 = fadd float %x, 2.000000e+00
  %4 = fadd float %y, 2.000000e+00
  %5 = fadd float %z, 2.000000e+00
  %6 = fadd float %0, %3 ; (x + a) + (x + 2)
  %7 = fadd float %1, %4 ; (y + a) + (y + 2)
  %8 = fadd float %2, %5 ; (z + a) + (z + 2) => a + 2 is common, calculate once
  %9 =  insertelement <3 x float> undef, float %6, i64 0
  %10 = insertelement <3 x float> %9,    float %7, i64 1
  %11 = insertelement <3 x float> %10,   float %8, i64 2
  ret <3 x float> %11
}

; CHECK-LABEL: define <3 x float> @test3
; CHECK-NOT: fadd float %x, %a
; CHECK-NOT: fadd float %y, %a
; CHECK-NOT: fadd float %z, %a
; CHECK: %0 = fadd float 2.000000e+00, %a
; CHECK: %1 = fadd float %x, %0
; CHECK: %2 = fadd float %y, %0
; CHECK: %3 = fadd float %z, %0
; CHECK: %4 = fadd float %x, %1
; CHECK: %5 = fadd float %y, %2
; CHECK: %6 = fadd float %z, %3

define <3 x float> @test4(float %a, float %x, float %y, float %z) #0 {
entry:
  %0 = fmul float %x, %a
  %1 = fmul float %y, %a
  %2 = fmul float %z, %a
  %3 = fmul float %x, 2.000000e+00
  %4 = fmul float %y, 2.000000e+00
  %5 = fmul float %z, 2.000000e+00
  %6 = fmul float %0, %3 ; (x * a) * (x * 2)
  %7 = fmul float %1, %4 ; (y * a) * (y * 2)
  %8 = fmul float %2, %5 ; (z * a) * (z * 2) => a * 2 is common, calculate once
  %9 =  insertelement <3 x float> undef, float %6, i64 0
  %10 = insertelement <3 x float> %9,    float %7, i64 1
  %11 = insertelement <3 x float> %10,   float %8, i64 2
  ret <3 x float> %11
}

; CHECK-LABEL: define <3 x float> @test4
; CHECK-NOT: fmul float %x, %a
; CHECK-NOT: fmul float %y, %a
; CHECK-NOT: fmul float %z, %a
; CHECK: %0 = fmul float 2.000000e+00, %a
; CHECK: %1 = fmul float %x, %0
; CHECK: %2 = fmul float %y, %0
; CHECK: %3 = fmul float %z, %0
; CHECK: %4 = fmul float %x, %1
; CHECK: %5 = fmul float %y, %2
; CHECK: %6 = fmul float %z, %3

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
