;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitFCmpInstFCmpFAddOp

; x + 2 >= 0
; can be
; x >= -2
define i1 @test1(float %x) #0 {
entry:
  %0 = fadd float %x, 2.000000e+00
  %1 = fcmp uge float %0, 0.000000e+00
  ret i1 %1
}

; CHECK-LABEL: define i1 @test1
; CHECK-NOT: fadd
; CHECK: %0 = fcmp uge float %x, -2.000000e+00
; CHECK: ret i1 %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
