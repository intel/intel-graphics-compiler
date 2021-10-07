;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-process-func-attributes -igc-set-fast-math-flags -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @testfinite1(float %a, float %b) {
  %1 = fadd float %a, %b
  %2 = fsub float %a, %b
  %3 = fmul float %a, %b
  %4 = fdiv float %a, %b
  %5 = frem float %a, %b
  ret void
}

; CHECK: testfinite1
; CHECK: fadd nnan ninf
; CHECK: fsub nnan ninf
; CHECK: fmul nnan ninf
; CHECK: fdiv nnan ninf
; CHECK: frem nnan ninf

define void @testfinite2(float %a, float %b) {
  %1 = fadd arcp float %a, %b
  %2 = fsub arcp float %a, %b
  %3 = fmul arcp float %a, %b
  %4 = fdiv arcp float %a, %b
  %5 = frem arcp float %a, %b
  ret void
}

; CHECK: testfinite2
; CHECK: fadd nnan ninf arcp
; CHECK: fsub nnan ninf arcp
; CHECK: fmul nnan ninf arcp
; CHECK: fdiv nnan ninf arcp
; CHECK: frem nnan ninf arcp

define void @testfinite3(float %a, float %b) {
  %1 = fadd nsz float %a, %b
  %2 = fsub nsz float %a, %b
  %3 = fmul nsz float %a, %b
  %4 = fdiv nsz float %a, %b
  %5 = frem nsz float %a, %b
  ret void
}

; CHECK: testfinite3
; CHECK: fadd nnan ninf nsz
; CHECK: fsub nnan ninf nsz
; CHECK: fmul nnan ninf nsz
; CHECK: fdiv nnan ninf nsz
; CHECK: frem nnan ninf nsz

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FiniteMathOnly", i1 true}
