;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: igc_opt -igc-set-fast-math-flags -S %s -o %t.ll
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
