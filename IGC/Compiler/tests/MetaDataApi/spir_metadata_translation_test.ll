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
; RUN: opt -igc-spir-metadata-translation -S %s -o %t.ll 
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

!0 = metadata !{void (i32)* @main, metadata !1, metadata !2, metadata !3, metadata !4, metadata !5, metadata !6, metadata !7, metadata !8, metadata !9}
!1 = metadata !{metadata !"work_group_size_hint", i32 1, i32 6, i32 4}
!2 = metadata !{metadata !"reqd_work_group_size", i32 4, i32 5, i32 1}
!3 = metadata !{metadata !"vec_type_hint", double undef, i1 true}
!4 = metadata !{metadata !"kernel_arg_addr_space", i32 2, i32 2}
!5 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none", metadata !"read_write"}
!6 = metadata !{metadata !"kernel_arg_type", metadata !"char*", metadata !"char*"}
!7 = metadata !{metadata !"kernel_arg_base_type", metadata !"char*", metadata !"char*"}
!8 = metadata !{metadata !"kernel_arg_type_qual", metadata !"const", metadata !"restrict"}
!9 = metadata !{metadata !"kernel_arg_name", metadata !"gooni", metadata !"googoo"}
!10 = metadata !{metadata !"-cl-mad-enable", metadata !"-cl-denorms-are-zero"}
!11 = metadata !{metadata !"-opt-arch-pdp11"}
!12 = metadata !{metadata !"-cl_doubles", metadata !"-cl_images"}
!13 = metadata !{metadata !"cl_khr_fp16", metadata !"cl_khr_int64_base_atomics"}
!14 = metadata !{i32 1, i32 2}
!15 = metadata !{i32 1, i32 1}

; CHECK-NOT: opencl.kernels
; CHECK-NOT: opencl.compiler.options
; CHECK-NOT: opencl.compiler.ext.options
; CHECK-NOT: opencl.enable.FP_CONTRACT
; CHECK-NOT: opencl.used.optional.core.features
; CHECK-NOT: opencl.used.extensions
; CHECK-NOT: opencl.spir.version
; CHECK-NOT: opencl.ocl.version

; CHECK: igc.version = !{![[V:[0-9]+]]
; CHECK: igc.input.ir = !{![[IIR:[0-9]+]]
; CHECK: igc.input.lang.info = !{![[IIR]]}
; CHECK: igc.functions = !{![[F:[0-9]+]]
; CHECK: igc.compiler.options = !{![[CO1:[0-9]+]], ![[CO2:[0-9]+]]}
; CHECK: igc.enable.FP_CONTRACT = !{}

; CHECK-NOT: opencl.kernels
; CHECK-NOT: opencl.compiler.options
; CHECK-NOT: opencl.compiler.ext.options
; CHECK-NOT: opencl.enable.FP_CONTRACT
; CHECK-NOT: opencl.used.optional.core.features
; CHECK-NOT: opencl.used.extensions
; CHECK-NOT: opencl.spir.version
; CHECK-NOT: opencl.ocl.version

; CHECK: ![[V]] = metadata !{i32 1, i32 0}
; CHECK: ![[IIR]] = metadata !{metadata !"ocl", i32 1, i32 1}
; CHECK: ![[F]] = metadata !{void (i32)* @main, metadata ![[FINFO:[0-9]+]]}
; CHECK: ![[FINFO]] = metadata !{metadata ![[FTYPE:[0-9]+]], metadata ![[TGZ:[0-9]+]], metadata ![[TGZH:[0-9]+]], metadata ![[VTH:[0-9]+]], metadata ![[KAAS:[0-9]+]], metadata ![[KAAQ:[0-9]+]], metadata ![[KAT:[0-9]+]], metadata ![[KABT:[0-9]+]], metadata ![[KATQ:[0-9]+]], metadata ![[KAN:[0-9]+]]
; CHECK: ![[FTYPE]] = metadata !{metadata !"function_type", i32 0}
; CHECK: ![[TGZ]] = metadata !{metadata !"thread_group_size", i32 4, i32 5, i32 1}
; CHECK: ![[TGZH]] = metadata !{metadata !"thread_group_size_hint", i32 1, i32 6, i32 4}
; CHECK: ![[VTH]] = metadata !{metadata !"opencl_vec_type_hint", double undef, i1 true}
; CHECK: ![[KAAS]] = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 2, i32 2}
; CHECK: ![[KAAQ]] = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_write"}
; CHECK: ![[KAT]] = metadata !{metadata !"opencl_kernel_arg_type", metadata !"char*", metadata !"char*"}
; CHECK: ![[KABT]] = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"char*", metadata !"char*"}
; CHECK: ![[KATQ]] = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"const", metadata !"restrict"}
; CHECK: ![[KAN]] = metadata !{metadata !"opencl_kernel_arg_name", metadata !"gooni", metadata !"googoo"}
; CHECK: ![[CO1]] = metadata !{metadata !"-mad-enable"}
; CHECK: ![[CO2]] = metadata !{metadata !"-denorms-are-zero"}
