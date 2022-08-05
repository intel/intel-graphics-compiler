;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; 0/x => 0
define float @div_to_zero(float %x) #0 {
entry:
  %0 = fdiv float 0.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @div_to_zero
; CHECK-NOT: fdiv
; CHECK: ret float 0.000000e+00

; x/1 => x
define float @div_by_one(float %x) #0 {
entry:
  %0 = fdiv float %x, 1.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @div_by_one
; CHECK-NOT: fdiv
; CHECK: ret float %x

; x/3 => x * 1/3
define float @div_to_mul(float %x) #0 {
entry:
  %0 = fdiv float %x, 3.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @div_to_mul
; CHECK-NOT: fdiv float %x, 3.000000e+00
; CHECK: %0 = fdiv float 1.000000e+00, 3.000000e+00
; CHECK: %1 = fmul float %x, %0
; CHECK: ret float %1

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
