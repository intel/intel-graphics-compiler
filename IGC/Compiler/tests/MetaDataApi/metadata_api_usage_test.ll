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
; RUN: igc_opt -igc-test-metadata-api-usage -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

@globalVar = external global i32

define void @main(i32 %x) nounwind {
  ret void
}

define float @f(float %x) nounwind {
	ret float %x
}

!igc.version = !{!0}
!igc.input.ir = !{!1}
!igc.input.lang.info = !{!2, !3}
!igc.functions = !{!4, !48}
!igc.compiler.options = !{!5, !6}
!igc.inline.constants = !{!25}
!igc.inline.globals = !{!28}
!igc.inline.programscope.offsets = !{!27}
!igc.global.pointer.info = !{!46, !51, !52}
!igc.constant.pointer.info = !{!47}
!igc.enable.FP_CONTRACT = !{}

!0 = !{i32 1, i32 1}
!1 = !{!"ocl", i32 1, i32 1}
!2 = !{!"ocl", i32 1, i32 0}
!3 = !{!"ogl", i32 1, i32 1}
!4 = !{void (i32)* @main, !21}
!21 = !{!7, !8, !40, !9, !10, !22, !23, !29, !39, !11, !15, !16, !17, !18, !19, !20}
!7 = !{!"function_type", i32 5}
!8 = !{!"arg_desc", !12}
!12 = !{i32 1}
!40 = !{!"implicit_arg_desc", !13}
!13 = !{i32 2, !14}
!14 = !{!"explicit_arg_num", i32 4}
!9 = !{!"thread_group_size", i32 1, i32 2, i32 3}
!10 = !{!"thread_group_size_hint", i32 3, i32 2, i32 1}
!22 = !{!"local_size", i32 100}
!23 = !{!"local_offsets", !24}
!24 = !{i32* @globalVar, i32 10}
!29 = !{!"resource_alloc", !30, !31, !32, !33, !36}
!30 = !{!"uavs_num", i32 4}
!31 = !{!"srvs_num", i32 1}
!32 = !{!"samplers_num", i32 7}
!33 = !{!"arg_allocs", !34, !35}
!34 = !{i32 2, i32 0, i32 4}
!35 = !{i32 1, i32 1, i32 2}
!36 = !{!"inline_samplers", !37, !38}
!37 = !{i32 7, i32 1}
!38 = !{i32 4, i32 2}
!39 = !{!"private_memory_per_wi", i32 123}
!11 = !{!"opencl_vec_type_hint", double undef, i1 true}
!15 = !{!"opencl_kernel_arg_addr_space", i32 2, i32 2}
!16 = !{!"opencl_kernel_arg_access_qual", !"none", !"read_write"}
!17 = !{!"opencl_kernel_arg_type", !"char*", !"char*"}
!18 = !{!"opencl_kernel_arg_base_type", !"char*", !"char*"}
!19 = !{!"opencl_kernel_arg_type_qual", !"const", !"restrict"}
!20 = !{!"opencl_kernel_arg_name", !"gooni", !"googoo"}
!48 = !{float (float)* @f, !49}
!49 = !{!50}
!50 = !{!"function_type", i32 3}
!5 = !{!"-cl-mad-enable"}
!6 = !{!"-cl-denorms-are-zero"}
!25 = !{!26, i32 6}
!26 = !{i8 4, i8 7}
!27 = !{i32* @globalVar, i32 20}
!28 = !{!45, i32 4}
!45 = !{i8 7, i8 8}
!46 = !{i32 0, i32 20, i32 2, i32 0}
!51 = !{i32 1, i32 2, i32 3, i32 4}
!52 = !{i32 4, i32 3, i32 2, i32 1}
!47 = !{i32 0, i32 40, i32 2, i32 0}

; CHECK: igc.version = !{}
; CHECK: igc.input.ir = !{![[IIR:[0-9]+]]
; CHECK: igc.input.lang.info = !{![[IL1:[0-9]+]], ![[IL2:[0-9]+]]}
; CHECK: igc.functions = !{![[F1:[0-9]+]], ![[F2:[0-9]+]]}
; CHECK: igc.compiler.options = !{![[CO1:[0-9]+]], ![[CO2:[0-9]+]]}
; CHECK: igc.inline.constants = !{![[IC:[0-9]+]]
; CHECK-NOT: igc.inline.globals = 
; CHECK: igc.inline.programscope.offsets = !{}
; CHECK: igc.global.pointer.info = !{![[GPI1:[0-9]+]], ![[GPI2:[0-9]+]], ![[GPI3:[0-9]+]]
; CHECK: igc.constant.pointer.info = !{![[CPI:[0-9]+]]
; CHECK-NOT: igc.enable.FP_CONTRACT = !{}

; CHECK: ![[IIR]] = !{!"ocl", i32 1, i32 1}
; CHECK: ![[IL1]] = !{!"ocl", i32 1, i32 0}
; CHECK: ![[IL2]] = !{!"ogl", i32 1, i32 1}
; CHECK: ![[F1]] = !{float (float)* @f, ![[FINFO1:[0-9]+]]}
; CHECK: ![[FINFO1]] = !{![[FTYPE1:[0-9]+]], ![[ARGDESC1:[0-9]+]], ![[IMPARGDESC1:[0-9]+]]}
; CHECK: ![[FTYPE1]] = !{!"function_type", i32 0}
; CHECK: ![[ARGDESC1]] = !{!"arg_desc", ![[ARG1:[0-9]+]]}
; CHECK: ![[ARG1]] = !{i32 4}
; CHECK: ![[IMPARGDESC1]] = !{!"implicit_arg_desc", ![[ARG2:[0-9]+]]}
; CHECK: ![[ARG2]] = !{i32 3, ![[EXPARG:[0-9]+]]
; CHECK: ![[EXPARG]] = !{!"explicit_arg_num", i32 4}
; CHECK: ![[F2]] = !{i32 ()* @gcd, ![[FINFO2:[0-9]+]]}
; CHECK: ![[FINFO2]] = !{![[FTYPE2:[0-9]+]]}
; CHECK: ![[FTYPE2]] = !{!"function_type", i32 3}
; CHECK: ![[CO1]] = !{!"-cl-mad-enable"}
; CHECK: ![[CO2]] = !{!"-cl-denorms-are-zero"}
; CHECK: ![[IC]] = !{![[BUFF:[0-9]+]], i32 6}
; CHECK: ![[BUFF]] = !{i8 4, i8 7}
; CHECK: ![[GPI1]] = !{i32 2, i32 2, i32 2, i32 2}
; CHECK: ![[GPI2]] = !{i32 4, i32 3, i32 15, i32 1}
; CHECK: ![[GPI3]] = !{i32 1, i32 1, i32 1, i32 1}
; CHECK: ![[CPI]] = !{i32 0, i32 40, i32 2, i32 0}
