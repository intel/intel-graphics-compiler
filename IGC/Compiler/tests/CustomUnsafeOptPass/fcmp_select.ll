;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitFCmpInstFCmpSelOp

define i1 @test1(float %x) #0 {
entry:
  %0 = fcmp ole float %x, 0.000000e+00
  %1 = select i1 %0, float 0.000000e+00, float 1.000000e+00
  %2 = fsub float -0.000000e+00, %1
  ; if x <= 0 (true), then second fcmp always true
  ; if x > 0 (false), then second fcmp always false
  ; both fcmp have the same result
  %3 = fcmp ueq float %1, %2
  ret i1 %3
}

; CHECK-LABEL: define i1 @test1
; CHECK-NOT: fsub
; CHECK-NOT: select
; CHECK: %0 = fcmp ole float %x, 0.000000e+00
; CHECK: ret i1 %0

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
