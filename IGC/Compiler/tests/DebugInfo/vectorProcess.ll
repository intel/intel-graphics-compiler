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
; RUN: igc_opt -igc-vectorprocess -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that vector process pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
define void @vectorProcess_test(<4 x i16> addrspace(1)* %src1, <4 x i16> addrspace(1)* %src2, <4 x i16> addrspace(1)* %dst) #0 {
  %1 = load <4 x i16>, <4 x i16> addrspace(1)* %src1, align 8, !dbg !3
  %scalar = extractelement <4 x i16> %1, i32 0
  %scalar1 = extractelement <4 x i16> %1, i32 1
  %scalar2 = extractelement <4 x i16> %1, i32 2
  %scalar3 = extractelement <4 x i16> %1, i32 3
  %2 = load <4 x i16>, <4 x i16> addrspace(1)* %src2, align 8, !dbg !4
  %scalar4 = extractelement <4 x i16> %2, i32 0
  %scalar5 = extractelement <4 x i16> %2, i32 1
  %scalar6 = extractelement <4 x i16> %2, i32 2
  %scalar7 = extractelement <4 x i16> %2, i32 3
  %3 = add i16 %scalar, %scalar4
  %4 = add i16 %scalar1, %scalar5
  %5 = add i16 %scalar2, %scalar6
  %6 = add i16 %scalar3, %scalar7
  %assembled.vect = insertelement <4 x i16> undef, i16 %3, i32 0
  %assembled.vect8 = insertelement <4 x i16> %assembled.vect, i16 %4, i32 1
  %assembled.vect9 = insertelement <4 x i16> %assembled.vect8, i16 %5, i32 2
  %assembled.vect10 = insertelement <4 x i16> %assembled.vect9, i16 %6, i32 3
  store <4 x i16> %assembled.vect10, <4 x i16> addrspace(1)* %dst, align 8, !dbg !5
  ret void
  
; CHECK: [[vptrcast:%[a-zA-Z0-9]+]] = bitcast <4 x i16> addrspace(1)* %src1 to <2 x i32> addrspace(1)*, !dbg !3
; CHECK-NEXT: [[vCastload:%[a-zA-Z0-9]+]] = load <2 x i32>, <2 x i32> addrspace(1)* [[vptrcast]], align 8, !dbg !3
; CHECK-NEXT: [[inst1:%[a-zA-Z0-9]+]] = bitcast <2 x i32> [[vCastload]] to <4 x i16>, !dbg !3
; CHECK: [[vptrcast1:%[a-zA-Z0-9]+]] = bitcast <4 x i16> addrspace(1)* %src2 to <2 x i32> addrspace(1)*, !dbg !4
; CHECK-NEXT: [[vCastload2:%[a-zA-Z0-9]+]] = load <2 x i32>, <2 x i32> addrspace(1)* [[vptrcast1]], align 8, !dbg !4
; CHECK-NEXT: [[inst2:%[a-zA-Z0-9]+]] = bitcast <2 x i32> [[vCastload2]] to <4 x i16>, !dbg !4
; CHECK: [[vptrcast3:%[a-zA-Z0-9]+]] = bitcast <4 x i16> addrspace(1)* %dst to <2 x i32> addrspace(1)*, !dbg !5
; CHECK-NEXT: [[inst3:%[a-zA-Z0-9]+]] = bitcast <4 x i16> [[inst4:%[a-zA-Z0-9.]+]] to <2 x i32>, !dbg !5
; CHECK-NEXT: store <2 x i32> [[inst3]], <2 x i32> addrspace(1)* [[vptrcast3]], align 8, !dbg !5
  
}

attributes #0 = { alwaysinline nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-frame-pointer-elim-non-leaf"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3 ,!4, !5}

!igc.functions = !{!2}

!0 = !{}
!1 = !{!0}
!2 = !{void (<4 x i16> addrspace(1)*, <4 x i16> addrspace(1)*, <4 x i16> addrspace(1)*)*@vectorProcess_test, !1}
!3 = !{i32 2, i32 0, !0, null}
!4 = !{i32 4, i32 0, !0, null}
!5 = !{i32 7, i32 0, !0, null}

