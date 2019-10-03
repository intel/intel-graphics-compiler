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

define void @testnsz1(float %a, float %b) {
  %1 = fadd float %a, %b
  %2 = fsub float %a, %b
  %3 = fmul float %a, %b
  %4 = fdiv float %a, %b
  %5 = frem float %a, %b
  ret void
}

; CHECK: testnsz1
; CHECK: fadd nsz
; CHECK: fsub nsz
; CHECK: fmul nsz
; CHECK: fdiv nsz
; CHECK: frem nsz

define void @testnsz2(float %a, float %b) {
  %1 = fadd arcp float %a, %b
  %2 = fsub arcp float %a, %b
  %3 = fmul arcp float %a, %b
  %4 = fdiv arcp float %a, %b
  %5 = frem arcp float %a, %b
  ret void
}

; CHECK: testnsz2
; CHECK: fadd nsz arcp
; CHECK: fsub nsz arcp
; CHECK: fmul nsz arcp
; CHECK: fdiv nsz arcp
; CHECK: frem nsz arcp

define void @testnsz3(float %a, float %b) {
  %1 = fadd nnan float %a, %b
  %2 = fsub nnan float %a, %b
  %3 = fmul nnan float %a, %b
  %4 = fdiv nnan float %a, %b
  %5 = frem nnan float %a, %b
  ret void
}

; CHECK: testnsz3
; CHECK: fadd nnan nsz
; CHECK: fsub nnan nsz
; CHECK: fmul nnan nsz
; CHECK: fdiv nnan nsz
; CHECK: frem nnan nsz

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"NoSignedZeros", i1 true}
