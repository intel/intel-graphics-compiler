;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-process-func-attributes -igc-set-fast-math-flags -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @testunsafe1(float %a, float %b) {
  %1 = fadd float %a, %b
  %2 = fsub float %a, %b
  %3 = fmul float %a, %b
  %4 = fdiv float %a, %b
  %5 = frem float %a, %b
  ret void
}

; CHECK: testunsafe1
; CHECK: fadd nsz
; CHECK: fsub nsz
; CHECK: fmul nsz
; CHECK: fdiv nsz
; CHECK: frem nsz

define void @testunsafe2(float %a, float %b) {
  %1 = fadd arcp float %a, %b
  %2 = fsub arcp float %a, %b
  %3 = fmul arcp float %a, %b
  %4 = fdiv arcp float %a, %b
  %5 = frem arcp float %a, %b
  ret void
}
; CHECK: testunsafe2
; CHECK: fadd nsz arcp
; CHECK: fsub nsz arcp
; CHECK: fmul nsz arcp
; CHECK: fdiv nsz arcp
; CHECK: frem nsz arcp

define void @testunsafe3(float %a, float %b) {
  %1 = fadd ninf float %a, %b
  %2 = fsub ninf float %a, %b
  %3 = fmul ninf float %a, %b
  %4 = fdiv ninf float %a, %b
  %5 = frem ninf float %a, %b
  ret void
}

; CHECK: testunsafe3
; CHECK: fadd ninf nsz
; CHECK: fsub ninf nsz
; CHECK: fmul ninf nsz
; CHECK: fdiv ninf nsz
; CHECK: frem ninf nsz

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UnsafeMathOptimizations", i1 true}
