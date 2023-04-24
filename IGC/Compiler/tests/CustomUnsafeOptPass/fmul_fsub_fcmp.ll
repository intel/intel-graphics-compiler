;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitFMulFCmpOp

; x * x => always >=0
define i1 @test1(float %x) #0 {
entry:
  %0 = fmul float %x, %x ; always >= 0
  %1 = fsub float 0.000000e+00, %0 ; always <= 0
  %2 = fcmp ogt float %0, %1 ; always x*x > -(x*x), change to x*x != 0
  ret i1 %2
}

; CHECK-LABEL: define i1 @test1
; CHECK-NOT: fmul
; CHECK-NOT: fsub
; CHECK: %0 = fcmp one float %x, 0
; CHECK: ret i1 %0

define i1 @test2(float %x) #0 {
entry:
  %0 = fmul float %x, -5.000000e+00 ; -x
  %1 = fsub float 0.000000e+00, %0 ; x
  %2 = fcmp ogt float %1, %0 ; x > -x, change to x > 0
  ret i1 %2
}

; CHECK-LABEL: define i1 @test2
; CHECK-NOT: fmul
; CHECK-NOT: fsub
; CHECK: %0 = fcmp ogt float %x, 0.000000e+00
; CHECK: ret i1 %0

define i1 @test3(float %x) #0 {
entry:
  %0 = fmul float %x, -5.000000e+00 ; -x
  %1 = fsub float 0.000000e+00, %0 ; x
  %2 = fcmp ogt float %0, %1 ; -x > x, change to 0 > x
  ret i1 %2
}

; CHECK-LABEL: define i1 @test3
; CHECK-NOT: fmul
; CHECK-NOT: fsub
; CHECK: %0 = fcmp ogt float 0.000000e+00, %x
; CHECK: ret i1 %0

define i1 @test4(float %x) #0 {
entry:
  %0 = fmul float %x, 5.000000e+00 ; x
  %1 = fsub float 0.000000e+00, %0 ; -x
  %2 = fcmp ogt float %1, %0 ; -x > x, change to 0 > x
  ret i1 %2
}

; CHECK-LABEL: define i1 @test4
; CHECK-NOT: fmul
; CHECK-NOT: fsub
; CHECK: %0 = fcmp ogt float 0.000000e+00, %x
; CHECK: ret i1 %0

define i1 @test5(float %x) #0 {
entry:
  %0 = fmul float %x, 5.000000e+00 ; x
  %1 = fsub float 0.000000e+00, %0 ; -x
  %2 = fcmp ogt float %0, %1 ; x > -x, change to x > 0
  ret i1 %2
}

; CHECK-LABEL: define i1 @test5
; CHECK-NOT: fmul
; CHECK-NOT: fsub
; CHECK: %0 = fcmp ogt float %x, 0.000000e+00
; CHECK: ret i1 %0

define float @test6(float %x) #0 {
entry:
  %0 = fmul float %x, 5.000000e+00
  %1 = fsub float 0.000000e+00, %0
  %2 = fcmp ogt float %0, %1 ; same as test5
  %3 = sitofp i1 %2 to float
  %4 = fmul float %1, %3 ; fmul/fsub have more uses, don't erase instructions
  ret float %4
}

; CHECK-LABEL: define float @test6
; CHECK: fsub
; CHECK: fmul
; CHECK: %2 = fcmp ogt float %x, 0.000000e+00

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
