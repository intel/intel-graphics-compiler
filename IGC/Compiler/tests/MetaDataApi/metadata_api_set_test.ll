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
; RUN: igc_opt -igc-test-metadata-api-set -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

@globalVar = external global i32

define void @main(i32 %x) nounwind {
  ret void
}

define void @f() nounwind {
  ret void
}

; CHECK: igc.version = !{![[V:[0-9]+]]
; CHECK: igc.input.ir = !{![[IIR:[0-9]+]]
; CHECK: igc.input.lang.info = !{![[IL1:[0-9]+]], ![[IL2:[0-9]+]]}
; CHECK: igc.functions = !{![[F1:[0-9]+]], ![[F2:[0-9]+]]}
; CHECK: igc.compiler.options = !{![[CO1:[0-9]+]], ![[CO2:[0-9]+]]}
; CHECK: igc.inline.constants = !{![[IC:[0-9]+]]
; CHECK: igc.inline.globals = !{![[GC:[0-9]+]]
; CHECK: igc.inline.programscope.offsets = !{![[ICO:[0-9]+]]
; CHECK: igc.global.pointer.info = !{![[GPI:[0-9]+]]
; CHECK: igc.constant.pointer.info = !{![[CPI:[0-9]+]]
; CHECK: igc.enable.FP_CONTRACT = !{}

; CHECK: ![[V]] = !{i32 1, i32 1}
; CHECK: ![[IIR]] = !{!"ocl", i32 1, i32 1}
; CHECK: ![[IL1]] = !{!"ocl", i32 1, i32 0}
; CHECK: ![[IL2]] = !{!"ogl", i32 1, i32 1}
; CHECK: ![[F1]] = !{void (i32)* @main, ![[FINFO1:[0-9]+]]}
; CHECK: ![[FINFO1]] = !{![[FTYPE:[0-9]+]], ![[ARGDESC:[0-9]+]], ![[IMPARGDESC:[0-9]+]], ![[TGZ:[0-9]+]], ![[TGZH:[0-9]+]], ![[LS:[0-9]+]], ![[LO:[0-9]+]], ![[RA:[0-9]+]], ![[PM:[0-9]+]], ![[VTH:[0-9]+]], ![[KAAS:[0-9]+]], ![[KAAQ:[0-9]+]], ![[KAT:[0-9]+]], ![[KABT:[0-9]+]], ![[KATQ:[0-9]+]], ![[KAN:[0-9]+]]
; CHECK: ![[FTYPE]] = !{!"function_type", i32 5}
; CHECK: ![[ARGDESC]] = !{!"arg_desc", ![[ARG1:[0-9]+]]}
; CHECK: ![[ARG1]] = !{i32 1
; CHECK: ![[IMPARGDESC]] = !{!"implicit_arg_desc", ![[ARG2:[0-9]+]]}
; CHECK: ![[ARG2]] = !{i32 2, ![[EXPARG:[0-9]+]]
; CHECK: ![[EXPARG]] = !{!"explicit_arg_num", i32 4}
; CHECK: ![[TGZ]] = !{!"thread_group_size", i32 1, i32 2, i32 3}
; CHECK: ![[TGZH]] = !{!"thread_group_size_hint", i32 3, i32 2, i32 1}
; CHECK: ![[LS]] = !{!"local_size", i32 100}
; CHECK: ![[LO]] = !{!"local_offsets", ![[LO1:[0-9]+]]}
; CHECK: ![[LO1]] = !{i32* @globalVar, i32 10}
; CHECK: ![[RA]] = !{!"resource_alloc", ![[UAV:[0-9]+]], ![[SRV:[0-9]+]], ![[SAMP:[0-9]+]], ![[ARGS:[0-9]+]], ![[ISS:[0-9]+]]}
; CHECK: ![[UAV]] = !{!"uavs_num", i32 4}
; CHECK: ![[SRV]] = !{!"srvs_num", i32 1}
; CHECK: ![[SAMP]] = !{!"samplers_num", i32 7}
; CHECK: ![[ARGS]] = !{!"arg_allocs", ![[ARG1:[0-9]+]], ![[ARG2:[0-9]+]]}
; CHECK: ![[ARG1]] = !{i32 2, i32 0, i32 4}
; CHECK: ![[ARG2]] = !{i32 1, i32 1, i32 2}
; CHECK: ![[ISS]] = !{!"inline_samplers", ![[IS1:[0-9]+]], ![[IS2:[0-9]+]]}
; CHECK: ![[IS1]] = !{i32 7, i32 1}
; CHECK: ![[IS2]] = !{i32 4, i32 2}
; CHECK: ![[PM]] = !{!"private_memory_per_wi", i32 123}
; CHECK: ![[VTH]] = !{!"opencl_vec_type_hint", double undef, i1 true}
; CHECK: ![[KAAS]] = !{!"opencl_kernel_arg_addr_space", i32 2, i32 2}
; CHECK: ![[KAAQ]] = !{!"opencl_kernel_arg_access_qual", !"none", !"read_write"}
; CHECK: ![[KAT]] = !{!"opencl_kernel_arg_type", !"char*", !"char*"}
; CHECK: ![[KABT]] = !{!"opencl_kernel_arg_base_type", !"char*", !"char*"}
; CHECK: ![[KATQ]] = !{!"opencl_kernel_arg_type_qual", !"const", !"restrict"}
; CHECK: ![[KAN]] = !{!"opencl_kernel_arg_name", !"gooni", !"googoo"}
; CHECK: ![[F2]] = !{void ()* @f, ![[FINFO2:[0-9]+]]}
; CHECK: ![[FINFO2]] = !{![[FTYPE2:[0-9]+]]}
; CHECK: ![[FTYPE2]] = !{!"function_type", i32 0}
; CHECK-NOT: thread_group_size
; CHECK-NOT: thread_group_size_hint
; CHECK-NOT: opencl_vec_type_hint
; CHECK-NOT: opencl_kernel_arg_addr_space
; CHECK-NOT: opencl_kernel_arg_access_qual
; CHECK-NOT: opencl_kernel_arg_type
; CHECK-NOT: opencl_kernel_arg_base_type
; CHECK-NOT: opencl_kernel_arg_type_qual
; CHECK-NOT: opencl_kernel_arg_name
; CHECK: ![[CO1]] = !{!"-cl-mad-enable"}
; CHECK: ![[CO2]] = !{!"-cl-denorms-are-zero"}
; CHECK: ![[IC]] = !{![[BUFF:[0-9]+]], i32 4}
; CHECK: ![[BUFF]] = !{i8 2, i8 5}
; CHECK: ![[GC]] = !{![[BUFF2:[0-9]+]], i32 4}
; CHECK: ![[BUFF2]] = !{i8 7, i8 8}
; CHECK: ![[ICO]] = !{i32* @globalVar, i32 15}
; CHECK: ![[GPI]] = !{i32 0, i32 20, i32 2, i32 0}
; CHECK: ![[CPI]] = !{i32 0, i32 40, i32 2, i32 0}


