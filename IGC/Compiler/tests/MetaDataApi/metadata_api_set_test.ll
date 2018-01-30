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
; RUN: opt -igc-test-metadata-api-set -S %s -o %t.ll 
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

; CHECK: ![[V]] = metadata !{i32 1, i32 1}
; CHECK: ![[IIR]] = metadata !{metadata !"ocl", i32 1, i32 1}
; CHECK: ![[IL1]] = metadata !{metadata !"ocl", i32 1, i32 0}
; CHECK: ![[IL2]] = metadata !{metadata !"ogl", i32 1, i32 1}
; CHECK: ![[F1]] = metadata !{void (i32)* @main, metadata ![[FINFO1:[0-9]+]]}
; CHECK: ![[FINFO1]] = metadata !{metadata ![[FTYPE:[0-9]+]], metadata ![[ARGDESC:[0-9]+]], metadata ![[IMPARGDESC:[0-9]+]], metadata ![[TGZ:[0-9]+]], metadata ![[TGZH:[0-9]+]], metadata ![[LS:[0-9]+]], metadata ![[LO:[0-9]+]], metadata ![[RA:[0-9]+]], metadata ![[PM:[0-9]+]], metadata ![[VTH:[0-9]+]], metadata ![[KAAS:[0-9]+]], metadata ![[KAAQ:[0-9]+]], metadata ![[KAT:[0-9]+]], metadata ![[KABT:[0-9]+]], metadata ![[KATQ:[0-9]+]], metadata ![[KAN:[0-9]+]]
; CHECK: ![[FTYPE]] = metadata !{metadata !"function_type", i32 5}
; CHECK: ![[ARGDESC]] = metadata !{metadata !"arg_desc", metadata ![[ARG1:[0-9]+]]}
; CHECK: ![[ARG1]] = metadata !{i32 1
; CHECK: ![[IMPARGDESC]] = metadata !{metadata !"implicit_arg_desc", metadata ![[ARG2:[0-9]+]]}
; CHECK: ![[ARG2]] = metadata !{i32 2, metadata ![[EXPARG:[0-9]+]]
; CHECK: ![[EXPARG]] = metadata !{metadata !"explicit_arg_num", i32 4}
; CHECK: ![[TGZ]] = metadata !{metadata !"thread_group_size", i32 1, i32 2, i32 3}
; CHECK: ![[TGZH]] = metadata !{metadata !"thread_group_size_hint", i32 3, i32 2, i32 1}
; CHECK: ![[LS]] = metadata !{metadata !"local_size", i32 100}
; CHECK: ![[LO]] = metadata !{metadata !"local_offsets", metadata ![[LO1:[0-9]+]]}
; CHECK: ![[LO1]] = metadata !{i32* @globalVar, i32 10}
; CHECK: ![[RA]] = metadata !{metadata !"resource_alloc", metadata ![[UAV:[0-9]+]], metadata ![[SRV:[0-9]+]], metadata ![[SAMP:[0-9]+]], metadata ![[ARGS:[0-9]+]], metadata ![[ISS:[0-9]+]]}
; CHECK: ![[UAV]] = metadata !{metadata !"uavs_num", i32 4}
; CHECK: ![[SRV]] = metadata !{metadata !"srvs_num", i32 1}
; CHECK: ![[SAMP]] = metadata !{metadata !"samplers_num", i32 7}
; CHECK: ![[ARGS]] = metadata !{metadata !"arg_allocs", metadata ![[ARG1:[0-9]+]], metadata ![[ARG2:[0-9]+]]}
; CHECK: ![[ARG1]] = metadata !{i32 2, i32 0, i32 4}
; CHECK: ![[ARG2]] = metadata !{i32 1, i32 1, i32 2}
; CHECK: ![[ISS]] = metadata !{metadata !"inline_samplers", metadata ![[IS1:[0-9]+]], metadata ![[IS2:[0-9]+]]}
; CHECK: ![[IS1]] = metadata !{i32 7, i32 1}
; CHECK: ![[IS2]] = metadata !{i32 4, i32 2}
; CHECK: ![[PM]] = metadata !{metadata !"private_memory_per_wi", i32 123}
; CHECK: ![[VTH]] = metadata !{metadata !"opencl_vec_type_hint", double undef, i1 true}
; CHECK: ![[KAAS]] = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 2, i32 2}
; CHECK: ![[KAAQ]] = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_write"}
; CHECK: ![[KAT]] = metadata !{metadata !"opencl_kernel_arg_type", metadata !"char*", metadata !"char*"}
; CHECK: ![[KABT]] = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"char*", metadata !"char*"}
; CHECK: ![[KATQ]] = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"const", metadata !"restrict"}
; CHECK: ![[KAN]] = metadata !{metadata !"opencl_kernel_arg_name", metadata !"gooni", metadata !"googoo"}
; CHECK: ![[F2]] = metadata !{void ()* @f, metadata ![[FINFO2:[0-9]+]]}
; CHECK: ![[FINFO2]] = metadata !{metadata ![[FTYPE2:[0-9]+]]}
; CHECK: ![[FTYPE2]] = metadata !{metadata !"function_type", i32 0}
; CHECK-NOT: thread_group_size
; CHECK-NOT: thread_group_size_hint
; CHECK-NOT: opencl_vec_type_hint
; CHECK-NOT: opencl_kernel_arg_addr_space
; CHECK-NOT: opencl_kernel_arg_access_qual
; CHECK-NOT: opencl_kernel_arg_type
; CHECK-NOT: opencl_kernel_arg_base_type
; CHECK-NOT: opencl_kernel_arg_type_qual
; CHECK-NOT: opencl_kernel_arg_name
; CHECK: ![[CO1]] = metadata !{metadata !"-cl-mad-enable"}
; CHECK: ![[CO2]] = metadata !{metadata !"-cl-denorms-are-zero"}
; CHECK: ![[IC]] = metadata !{metadata ![[BUFF:[0-9]+]], i32 4}
; CHECK: ![[BUFF]] = metadata !{i8 2, i8 5}
; CHECK: ![[GC]] = metadata !{metadata ![[BUFF2:[0-9]+]], i32 4}
; CHECK: ![[BUFF2]] = metadata !{i8 7, i8 8}
; CHECK: ![[ICO]] = metadata !{i32* @globalVar, i32 15}
; CHECK: ![[GPI]] = metadata !{i32 0, i32 20, i32 2, i32 0}
; CHECK: ![[CPI]] = metadata !{i32 0, i32 40, i32 2, i32 0}


