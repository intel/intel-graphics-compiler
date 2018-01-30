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
; RUN: igc_opt %s -S -o - -igc-type-legalizer -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @sroa(<4 x i64> addrspace(1)* %src, <4 x i64> addrspace(1)* %dst) {
  %s0 = load <4 x i64> addrspace(1)* %src, align 32
  %s1 = bitcast <4 x i64> %s0 to i256
  %s2 = and i256 %s1, -280375465082881
  %s3 = or i256 %s2, 72567767433216
  %s4 = bitcast i256 %s3 to <4 x i64>
  store <4 x i64> %s4, <4 x i64> addrspace(1)* %dst, align 32
  ret void
}


; CHECK-LABEL: define void @sroa
; CHECK: %src.sclr0.ptrcast = bitcast <4 x i64> addrspace(1)* %src to <2 x i64> addrspace(1)*
; CHECK: %s0.vec0 = load <2 x i64> addrspace(1)* %src.sclr0.ptrcast, align 32
; CHECK: %s0.sclr0 = extractelement <2 x i64> %s0.vec0, i32 0
; CHECK: %src.sclr2 = getelementptr inbounds <4 x i64> addrspace(1)* %src, i32 0, i32 2
; CHECK: %src.sclr2.ptrcast = bitcast i64 addrspace(1)* %src.sclr2 to <2 x i64> addrspace(1)*
; CHECK: %s0.vec2 = load <2 x i64> addrspace(1)* %src.sclr2.ptrcast, align 32
; CHECK: %s2.ex0 = and i64 %s0.sclr0, -280375465082881
; CHECK: %s3.ex0.or = or i64 %s2.ex0, 72567767433216
; CHECK: %dst.sclr0.ptrcast = bitcast <4 x i64> addrspace(1)* %dst to <2 x i64> addrspace(1)*
; CHECK: %s4.vec1.vec0 = insertelement <2 x i64> undef, i64 %s3.ex0.or, i32 0
; CHECK: %s4.vec1.vec1 = shufflevector <2 x i64> %s4.vec1.vec0, <2 x i64> %s0.vec0, <2 x i32> <i32 0, i32 3>
; CHECK: store <2 x i64> %s4.vec1.vec1, <2 x i64> addrspace(1)* %dst.sclr0.ptrcast, align 32
; CHECK: %dst.sclr2 = getelementptr inbounds <4 x i64> addrspace(1)* %dst, i32 0, i32 2
; CHECK: %dst.sclr2.ptrcast = bitcast i64 addrspace(1)* %dst.sclr2 to <2 x i64> addrspace(1)*
; CHECK: store <2 x i64> %s0.vec2, <2 x i64> addrspace(1)* %dst.sclr2.ptrcast, align 32
; CHECK: ret void
