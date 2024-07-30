;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests visitFCmpInst

; uno (either NaNs) can't be true if NoNaNs is enabled
define i1 @test1(float %x, float %y) #0 {
entry:
  %0 = fcmp uno float %x, %y
  ret i1 %0
}

; CHECK-LABEL: define i1 @test1
; CHECK-NOT: fcmp
; CHECK: ret i1 false

; ord (no NaNs) can't be false if NoNaNs is enabled
define i1 @test2(float %x, float %y) #0 {
entry:
  %0 = fcmp ord float %x, %y
  ret i1 %0
}

; CHECK-LABEL: define i1 @test2
; CHECK-NOT: fcmp
; CHECK: ret i1 true

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"NoNaNs", i1 true}
