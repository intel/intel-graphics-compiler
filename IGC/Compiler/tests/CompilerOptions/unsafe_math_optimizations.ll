;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-process-func-attributes -igc-set-fast-math-flags -S %s -o %t.ll
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
; CHECK: fadd reassoc nsz arcp contract afn
; CHECK: fsub reassoc nsz arcp contract afn
; CHECK: fmul reassoc nsz arcp contract afn
; CHECK: fdiv reassoc nsz arcp contract afn
; CHECK: frem reassoc nsz arcp contract afn

define void @testunsafe2(float %a, float %b) {
  %1 = fadd arcp float %a, %b
  %2 = fsub arcp float %a, %b
  %3 = fmul arcp float %a, %b
  %4 = fdiv arcp float %a, %b
  %5 = frem arcp float %a, %b
  ret void
}
; CHECK: testunsafe2
; CHECK: fadd reassoc nsz arcp contract afn
; CHECK: fsub reassoc nsz arcp contract afn
; CHECK: fmul reassoc nsz arcp contract afn
; CHECK: fdiv reassoc nsz arcp contract afn
; CHECK: frem reassoc nsz arcp contract afn

define void @testunsafe3(float %a, float %b) {
  %1 = fadd ninf float %a, %b
  %2 = fsub ninf float %a, %b
  %3 = fmul ninf float %a, %b
  %4 = fdiv ninf float %a, %b
  %5 = frem ninf float %a, %b
  ret void
}

; CHECK: testunsafe3
; CHECK: fadd reassoc ninf nsz arcp contract afn
; CHECK: fsub reassoc ninf nsz arcp contract afn
; CHECK: fmul reassoc ninf nsz arcp contract afn
; CHECK: fdiv reassoc ninf nsz arcp contract afn
; CHECK: frem reassoc ninf nsz arcp contract afn

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UnsafeMathOptimizations", i1 true}
