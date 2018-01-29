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
; RUN: igc_opt -igc-correctly-rounded-div-sqrt -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

define float @testsqrt1(float %in) {
  %retVal = call float @_Z4sqrtf(float %in)
  ret float %retVal
}
; CHECK: testsqrt1
; CHECK: sqrt_cr

define <2 x float> @testsqrt2(<2 x float> %in) {
  %retVal = call <2 x float> @_Z4sqrtDv2_f(<2 x float> %in)
  ret <2 x float> %retVal
}
; CHECK: testsqrt2
; CHECK: sqrt_cr

define <3 x float> @testsqrt3(<3 x float> %in) {
  %retVal = call <3 x float> @_Z4sqrtDv3_f(<3 x float> %in)
  ret <3 x float> %retVal
}
; CHECK: testsqrt3
; CHECK: sqrt_cr

define <4 x float> @testsqrt4(<4 x float> %in) {
  %retVal = call <4 x float> @_Z4sqrtDv4_f(<4 x float> %in)
  ret <4 x float> %retVal
}
; CHECK: testsqrt4
; CHECK: sqrt_cr

define <8 x float> @testsqrt8(<8 x float> %in) {
  %retVal = call <8 x float> @_Z4sqrtDv8_f(<8 x float> %in)
  ret <8 x float> %retVal
}
; CHECK: testsqrt8
; CHECK: sqrt_cr

define <16 x float> @testsqrt16(<16 x float> %in) {
  %retVal = call <16 x float> @_Z4sqrtDv16_f(<16 x float> %in)
  ret <16 x float> %retVal
}
; CHECK: testsqrt16
; CHECK: sqrt_cr

define float @testdiv1(float %in1, float %in2) {
  %retVal = fdiv float %in1, %in2;
  ret float %retVal
}
; CHECK: testdiv1
; CHECK: divide_cr

define <2 x float> @testdiv2(<2 x float> %in1, <2 x float> %in2) {
  %retVal = fdiv <2 x float> %in1, %in2
  ret <2 x float> %retVal
}
; CHECK: testdiv2
; CHECK: divide_cr

define <3 x float> @testdiv3(<3 x float> %in1, <3 x float> %in2) {
  %retVal = fdiv <3 x float> %in1, %in2
  ret <3 x float> %retVal
}
; CHECK: testdiv3
; CHECK: divide_cr

define <4 x float> @testdiv4(<4 x float> %in1, <4 x float> %in2) {
  %retVal = fdiv <4 x float> %in1, %in2
  ret <4 x float> %retVal
}
; CHECK: testdiv4
; CHECK: divide_cr

define <8 x float> @testdiv8(<8 x float> %in1, <8 x float> %in2) {
  %retVal = fdiv <8 x float> %in1, %in2
  ret <8 x float> %retVal
}
; CHECK: testdiv8
; CHECK: divide_cr

define <16 x float> @testdiv16(<16 x float> %in1, <16 x float> %in2) {
  %retVal = fdiv <16 x float> %in1, %in2
  ret <16 x float> %retVal
}
; CHECK: testdiv16
; CHECK: divide_cr

declare float @_Z4sqrtf(float)
declare <2 x float> @_Z4sqrtDv2_f(<2 x float>)
declare <3 x float> @_Z4sqrtDv3_f(<3 x float>)
declare <4 x float> @_Z4sqrtDv4_f(<4 x float>)
declare <8 x float> @_Z4sqrtDv8_f(<8 x float>)
declare <16 x float> @_Z4sqrtDv16_f(<16 x float>)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"CorrectlyRoundedDivSqrt", i1 true}
