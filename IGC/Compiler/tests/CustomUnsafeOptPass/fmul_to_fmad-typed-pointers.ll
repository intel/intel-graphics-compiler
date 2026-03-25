;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; a*(1 - x) = a - a*x
define float @visitBinaryOperatorFmulToFmad(float %x) #0 {
entry:
  %0 = fsub float 1.000000e+00, %x
  %1 = fmul float %0, 5.000000e+00
  ret float %1
}

; CHECK-LABEL: define float @visitBinaryOperatorFmulToFmad
; CHECK-NOT: fsub float 1.000000e+00, %x
; CHECK: %0 = fmul float 5.000000e+00, %x
; CHECK: %1 = fsub float 5.000000e+00, %0
; CHECK: ret float %1

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
