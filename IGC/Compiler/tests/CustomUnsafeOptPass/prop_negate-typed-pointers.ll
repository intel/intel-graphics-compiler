;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitBinaryOperatorPropNegate

; -x + y = y -x
define float @test1(float %x, float %y) #0 {
entry:
  %0 = fsub float 0.000000e+00, %x
  %1 = fadd float %0, %y
  ret float %1
}

; CHECK-LABEL: define float @test1
; CHECK-NOT: fadd
; CHECK: %0 = fsub float %y, %x
; CHECK: ret float %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
