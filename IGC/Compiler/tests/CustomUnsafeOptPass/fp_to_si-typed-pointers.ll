;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitFPToSIInst

declare float @llvm.floor.f32(float)

; floor(1.0 + 0.0 + 0.5) => 1.0 + 0.0
define i32 @test(float %x) #0 {
entry:
  %0 = select i1 true, float 1.000000e+00, float 0.000000e+00
  %1 = select i1 false, float 1.000000e+00, float 0.000000e+00
  %2 = fadd fast float %0, %1
  %3 = fadd fast float %2, 5.000000e-01
  %4 = call fast float @llvm.floor.f32(float %3) ; always returns unchanged value, remove
  %5 = fptosi float %4 to i32
  ret i32 %5
}

; CHECK-LABEL: define i32 @test
; CHECK-NOT: fadd {{.*}} 5.000000e-01
; CHECK-NOT: @llvm.floor.f32
; CHECK: %0 = select i1 true, float 1.000000e+00, float 0.000000e+00
; CHECK: %1 = select i1 false, float 1.000000e+00, float 0.000000e+00
; CHECK: %2 = fadd fast float %0, %1
; CHECK: %3 = fptosi float %2 to i32
; CHECK: ret i32 %3

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
