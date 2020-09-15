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
; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @sample_test(i32 %x, i32 %y, i1 %cond, float addrspace(1)* nocapture %res) nounwind {
entry:
  %cmp = icmp slt i32 %x, %y
  %cond.not = xor i1 %cond, true
  %and.cond = and i1 %cmp, %cond.not
  br i1 %and.cond, label %bb1, label %bb2
bb1:
  store float 0.0, float addrspace(1)* %res
  br label %bb3
bb2:
  store float 1.0, float addrspace(1)* %res
  br label %bb3
bb3:
  ret void
}


; CHECK:         [[CONDNEW:%[a-zA-Z0-9]+]] = icmp sge i32 %x, %y
; CHECK-NOT:     and
; CHECK:         [[ORRES:%[a-zA-Z0-9]+]] = or i1 %cond, [[CONDNEW]]
; CHECK:         br i1 [[ORRES:%[a-zA-Z0-9]+]], label %bb2, label %bb1
; CHECK-NOT:     br i1 {{.*}}, label %bb1, label %bb2

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
