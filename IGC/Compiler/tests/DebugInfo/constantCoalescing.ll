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
; RUN: igc_opt -igc-constant-coalescing -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that Constant Coalescing pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
define void @constantCoalescing_test(<4 x float> addrspace(1)* %results, <4 x float> addrspace(2)* %src1)  {
  %1 = load <4 x float>, <4 x float> addrspace(2)* %src1, align 16, !dbg !4
  %scalar = extractelement <4 x float> %1, i32 0, !dbg !5
  %scalar10 = extractelement <4 x float> %1, i32 1, !dbg !6
  %scalar11 = extractelement <4 x float> %1, i32 2, !dbg !7
  %scalar12 = extractelement <4 x float> %1, i32 3, !dbg !8
  %2 = ptrtoint <4 x float> addrspace(2)* %src1 to i32, !dbg !9
  %3 = add i32 %2, 16, !dbg !10
  %4 = inttoptr i32 %3 to <4 x float> addrspace(2)*, !dbg !11
  %5 = load <4 x float>, <4 x float> addrspace(2)* %4, align 16, !dbg !12
  %scalar13 = extractelement <4 x float> %5, i32 0, !dbg !13
  %scalar14 = extractelement <4 x float> %5, i32 1, !dbg !14
  %scalar15 = extractelement <4 x float> %5, i32 2, !dbg !15
  %scalar16 = extractelement <4 x float> %5, i32 3, !dbg !16
  %6 = add i32 %2, 32, !dbg !17
  %7 = inttoptr i32 %6 to <4 x float> addrspace(2)*, !dbg !18
  %8 = load <4 x float>, <4 x float> addrspace(2)* %7, align 16, !dbg !19
  %scalar17 = extractelement <4 x float> %8, i32 0, !dbg !20
  %scalar18 = extractelement <4 x float> %8, i32 1, !dbg !21
  %scalar19 = extractelement <4 x float> %8, i32 2, !dbg !22
  %scalar20 = extractelement <4 x float> %8, i32 3, !dbg !23
  %9 = add i32 %2, 48, !dbg !24
  %10 = inttoptr i32 %9 to <4 x float> addrspace(2)*
  %11 = load <4 x float>, <4 x float> addrspace(2)* %10, align 16
  %scalar21 = extractelement <4 x float> %11, i32 0
  %scalar22 = extractelement <4 x float> %11, i32 1
  %scalar23 = extractelement <4 x float> %11, i32 2
  %scalar24 = extractelement <4 x float> %11, i32 3
  %12 = fmul float %scalar, 3.000000e+00
  %13 = fmul float %scalar10, 3.000000e+00
  %14 = fmul float %scalar11, 3.000000e+00
  %15 = fmul float %scalar12, 3.000000e+00
  %assembled.vect = insertelement <4 x float> undef, float %12, i32 0
  %assembled.vect25 = insertelement <4 x float> %assembled.vect, float %13, i32 1
  %assembled.vect26 = insertelement <4 x float> %assembled.vect25, float %14, i32 2
  %assembled.vect27 = insertelement <4 x float> %assembled.vect26, float %15, i32 3
  store <4 x float> %assembled.vect27, <4 x float> addrspace(1)* %results, align 16
  %16 = fmul float %scalar13, 3.000000e+00
  %17 = fmul float %scalar14, 3.000000e+00
  %18 = fmul float %scalar15, 3.000000e+00
  %19 = fmul float %scalar16, 3.000000e+00
  %assembled.vect28 = insertelement <4 x float> undef, float %16, i32 0
  %assembled.vect29 = insertelement <4 x float> %assembled.vect28, float %17, i32 1
  %assembled.vect30 = insertelement <4 x float> %assembled.vect29, float %18, i32 2
  %assembled.vect31 = insertelement <4 x float> %assembled.vect30, float %19, i32 3
  %20 = ptrtoint <4 x float> addrspace(1)* %results to i32
  %21 = add i32 %20, 16
  %22 = inttoptr i32 %21 to <4 x float> addrspace(1)*
  store <4 x float> %assembled.vect31, <4 x float> addrspace(1)* %22, align 16
  %23 = fmul float %scalar17, 3.000000e+00
  %24 = fmul float %scalar18, 3.000000e+00
  %25 = fmul float %scalar19, 3.000000e+00
  %26 = fmul float %scalar20, 3.000000e+00
  %assembled.vect32 = insertelement <4 x float> undef, float %23, i32 0
  %assembled.vect33 = insertelement <4 x float> %assembled.vect32, float %24, i32 1
  %assembled.vect34 = insertelement <4 x float> %assembled.vect33, float %25, i32 2
  %assembled.vect35 = insertelement <4 x float> %assembled.vect34, float %26, i32 3
  %27 = add i32 %20, 32
  %28 = inttoptr i32 %27 to <4 x float> addrspace(1)*
  store <4 x float> %assembled.vect35, <4 x float> addrspace(1)* %28, align 16
  %29 = fmul float %scalar21, 3.000000e+00
  %30 = fmul float %scalar22, 3.000000e+00
  %31 = fmul float %scalar23, 3.000000e+00
  %32 = fmul float %scalar24, 3.000000e+00
  %assembled.vect36 = insertelement <4 x float> undef, float %29, i32 0
  %assembled.vect37 = insertelement <4 x float> %assembled.vect36, float %30, i32 1
  %assembled.vect38 = insertelement <4 x float> %assembled.vect37, float %31, i32 2
  %assembled.vect39 = insertelement <4 x float> %assembled.vect38, float %32, i32 3
  %33 = add i32 %20, 48
  %34 = inttoptr i32 %33 to <4 x float> addrspace(1)*
  store <4 x float> %assembled.vect39, <4 x float> addrspace(1)* %34, align 16
  ret void
  
; CHECK: [[inst1:%[a-zA-Z0-9]+]] = ptrtoint <4 x float> addrspace(2)* %src1 to i32, !dbg !4
; CHECK: [[inst2:%[a-zA-Z0-9]+]] = inttoptr i32 [[inst1]] to <16 x float> addrspace(2)*, !dbg !4
; CHECK: [[inst3:%[a-zA-Z0-9]+]] = load <16 x float>, <16 x float> addrspace(2)* [[inst2]], align 4, !dbg !4
; CHECK: [[inst4:%[a-zA-Z0-9]+]] = load <4 x float>, <4 x float> addrspace(2)* %src1, align 16, !dbg !4
; CHECK: [[scalar1:%[a-zA-Z0-9]+]] = extractelement <16 x float> [[inst3]], i32 0, !dbg !5
; CHECK-NOT: [[scalar2:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[inst4]]
; CHECK: [[scalar13:%[a-zA-Z0-9]+]] = extractelement <16 x float> [[inst3]], i32 4, !dbg !13
; CHECK-NOT: [[scalar14:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[scalar2:%[a-zA-Z0-9]+]], i32 5, !dbg !14

}
;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24}

!igc.functions = !{!3}
!0 = !{}
!1 = !{!"function_type", i32 0}
!2 = !{!1}
!3 = !{void (<4 x float> addrspace(1)*, <4 x float> addrspace(2)*)* @constantCoalescing_test, !2}
!4 = !{i32 1, i32 0, !0, null}
!5 = !{i32 3, i32 0, !0, null}
!6 = !{i32 5, i32 0, !0, null}
!7 = !{i32 6, i32 0, !0, null}
!8 = !{i32 11, i32 0, !0, null}
!9 = !{i32 13, i32 0, !0, null}
!10 = !{i32 15, i32 0, !0, null}
!11 = !{i32 16, i32 0, !0, null}
!12 = !{i32 21, i32 0, !0, null}
!13 = !{i32 23, i32 0, !0, null}
!14 = !{i32 25, i32 0, !0, null}
!15 = !{i32 26, i32 0, !0, null}
!16 = !{i32 31, i32 0, !0, null}
!17 = !{i32 33, i32 0, !0, null}
!18 = !{i32 35, i32 0, !0, null}
!19 = !{i32 36, i32 0, !0, null}
!20 = !{i32 41, i32 0, !0, null}
!21 = !{i32 43, i32 0, !0, null}
!22 = !{i32 45, i32 0, !0, null}
!23 = !{i32 46, i32 0, !0, null}
!24 = !{i32 51, i32 0, !0, null}
