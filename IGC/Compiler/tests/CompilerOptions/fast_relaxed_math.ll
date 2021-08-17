;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-process-func-attributes -igc-set-fast-math-flags -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @testfastrelaxed(float %a, float %b) {
  %1 = fadd float %a, %b
  %2 = fsub float %a, %b
  %3 = fmul float %a, %b
  %4 = fdiv float %a, %b
  %5 = frem float %a, %b
  ret void
}

; CHECK: fadd fast
; CHECK: fsub fast
; CHECK: fmul fast
; CHECK: fdiv fast
; CHECK: frem fast

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
