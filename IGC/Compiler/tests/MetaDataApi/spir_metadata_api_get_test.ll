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
; RUN: igc_opt -analyze -igc-test-spir-metadata-api-get -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

define void @main(i32 %x) nounwind {
  ret void
}

!opencl.kernels = !{!0}
!opencl.compiler.options = !{!10}
!opencl.compiler.ext.options = !{!11}
!opencl.enable.FP_CONTRACT = !{}
!opencl.used.optional.core.features = !{!12}
!opencl.used.extensions = !{!13}
!opencl.spir.version = !{!14}
!opencl.ocl.version = !{!15}

!0 = !{void (i32)* @main, !1, !2, !3, !4, !5, !6, !7, !8, !9}
!1 = !{!"work_group_size_hint", i32 1, i32 6, i32 4}
!2 = !{!"reqd_work_group_size", i32 4, i32 5, i32 1}
!3 = !{!"vec_type_hint", double undef, i1 true}
!4 = !{!"kernel_arg_addr_space", i32 2, i32 2}
!5 = !{!"kernel_arg_access_qual", !"none", !"read_write"}
!6 = !{!"kernel_arg_type", !"char*", !"char*"}
!7 = !{!"kernel_arg_base_type", !"char*", !"char*"}
!8 = !{!"kernel_arg_type_qual", !"const", !"restrict"}
!9 = !{!"kernel_arg_name", !"gooni", !"googoo"}
!10 = !{!"-cl-mad-enable", !"-cl-denorms-are-zero"}
!11 = !{!"-opt-arch-pdp11"}
!12 = !{!"cl_doubles", !"cl_images"}
!13 = !{!"cl_khr_fp16", !"cl_khr_int64_base_atomics"}
!14 = !{i32 1, i32 2}
!15 = !{i32 1, i32 1}

; CHECK: Kernels
; CHECK-NEXT: main
; CHECK-NEXT: Work Group Size Hint: 1,6,4
; CHECK-NEXT: Required Work Group Size: 4,5,1
; CHECK-NEXT: Vector Type Hint: 3, 1
; CHECK-NEXT: Kernel Arg Info
; CHECK-NEXT: Address Spaces
; CHECK-NEXT: 2 2
; CHECK-NEXT: Access Quialifiers
; CHECK-NEXT: none read_write
; CHECK-NEXT: Types
; CHECK-NEXT: char* char*
; CHECK-NEXT: Base Types
; CHECK-NEXT: char* char*
; CHECK-NEXT: Type Qualifiers
; CHECK-NEXT: const restrict
; CHECK-NEXT: Names
; CHECK-NEXT: gooni googoo
; CHECK-NEXT: Compiler Options
; CHECK-NEXT: -cl-mad-enable
; CHECK-NEXT: -cl-denorms-are-zero
; CHECK-NEXT: External Compiler Options
; CHECK-NEXT: -opt-arch-pdp11
; CHECK-NEXT: Floating Point Contractions
; CHECK-NEXT: 1
; CHECK-NEXT: Used Optional Core Features
; CHECK-NEXT: cl_doubles
; CHECK-NEXT: cl_images 
; CHECK-NEXT: Used KHR Extensions
; CHECK-NEXT: cl_khr_fp16
; CHECK-NEXT: cl_khr_int64_base_atomics
; CHECK-NEXT: SPIR Version
; CHECK-NEXT: 1.2
; CHECK-NEXT: OpenCL Version
; CHECK-NEXT: 1.1
