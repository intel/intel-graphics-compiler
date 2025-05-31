;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-custom-unsafe-opt-pass -S %s -o %t.ll
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
define float @div_add_arcp(float %x) #0 {
entry:
  %0 = fdiv float %x, 3.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @div_add_arcp
; CHECK: %0 = fdiv arcp float %x, 3.000000e+00
; CHECK: ret float %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
