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
; RUN: igc_opt -igc-spir-metadata-translation -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

target triple = "igil_32_GEN9"

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
!12 = !{!"-cl_doubles", !"-cl_images"}
!13 = !{!"cl_khr_fp16", !"cl_khr_int64_base_atomics"}
!14 = !{i32 1, i32 2}
!15 = !{i32 1, i32 1}

; CHECK-NOT: opencl.kernels
; CHECK-NOT: opencl.compiler.options
; CHECK-NOT: opencl.compiler.ext.options
; CHECK-NOT: opencl.enable.FP_CONTRACT
; CHECK-NOT: opencl.used.optional.core.features
; CHECK-NOT: opencl.used.extensions
; CHECK-NOT: opencl.spir.version
; CHECK-NOT: opencl.ocl.version

; CHECK: igc.input.ir = !{![[IIR:[0-9]+]]
; CHECK: igc.functions = !{![[F:[0-9]+]]

; CHECK-NOT: opencl.kernels
; CHECK-NOT: opencl.compiler.options
; CHECK-NOT: opencl.compiler.ext.options
; CHECK-NOT: opencl.enable.FP_CONTRACT
; CHECK-NOT: opencl.used.optional.core.features
; CHECK-NOT: opencl.used.extensions
; CHECK-NOT: opencl.spir.version
; CHECK-NOT: opencl.ocl.version

; CHECK: ![[IIR]] = !{!"ocl", i32 1, i32 1}
; CHECK: ![[F]] = !{void (i32)* @main, ![[FINFO:[0-9]+]]}
; CHECK: ![[FINFO]] = !{![[FTYPE:[0-9]+]], ![[TGZ:[0-9]+]], ![[TGZH:[0-9]+]], ![[VTH:[0-9]+]], ![[KAAS:[0-9]+]], ![[KAAQ:[0-9]+]], ![[KAT:[0-9]+]], ![[KABT:[0-9]+]], ![[KATQ:[0-9]+]], ![[KAN:[0-9]+]]
; CHECK: ![[FTYPE]] = !{!"function_type", i32 0}
; CHECK: ![[TGZ]] = !{!"thread_group_size", i32 4, i32 5, i32 1}
; CHECK: ![[TGZH]] = !{!"thread_group_size_hint", i32 1, i32 6, i32 4}
; CHECK: ![[VTH]] = !{!"opencl_vec_type_hint", double undef, i1 true}
; CHECK: ![[KAAS]] = !{!"opencl_kernel_arg_addr_space", i32 2, i32 2}
; CHECK: ![[KAAQ]] = !{!"opencl_kernel_arg_access_qual", !"none", !"read_write"}
; CHECK: ![[KAT]] = !{!"opencl_kernel_arg_type", !"char*", !"char*"}
; CHECK: ![[KABT]] = !{!"opencl_kernel_arg_base_type", !"char*", !"char*"}
; CHECK: ![[KATQ]] = !{!"opencl_kernel_arg_type_qual", !"const", !"restrict"}
; CHECK: ![[KAN]] = !{!"opencl_kernel_arg_name", !"gooni", !"googoo"}
