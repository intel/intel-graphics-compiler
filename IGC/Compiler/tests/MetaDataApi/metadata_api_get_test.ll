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
; RUN: opt -analyze -igc-test-metadata-api-get -S %s -o %t.ll 
; RUN: FileCheck %s --input-file=%t.ll

@globalVar = external global i32

define void @main(i32 %x) nounwind {
  ret void
}

!igc.version = !{!0}
!igc.input.ir = !{!1}
!igc.input.lang.info = !{!2, !3}
!igc.functions = !{!4}
!igc.compiler.options = !{!5, !6}
!igc.inline.constants = !{!25}
!igc.inline.globals = !{!28}
!igc.inline.programscope.offsets = !{!27}
!igc.global.pointer.info = !{!46}
!igc.constant.pointer.info = !{!47}
!igc.enable.FP_CONTRACT = !{}

!0 = metadata !{i32 1, i32 1}
!1 = metadata !{metadata !"ocl", i32 1, i32 1}
!2 = metadata !{metadata !"ocl", i32 1, i32 0}
!3 = metadata !{metadata !"ogl", i32 1, i32 1}
!4 = metadata !{void (i32)* @main, metadata !21}
!21 = metadata !{metadata !7, metadata !8, metadata !40, metadata !9, metadata !10, metadata !22, metadata !23, metadata !29, metadata !39, metadata !11, metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20}
!7 = metadata !{metadata !"function_type", i32 5}
!8 = metadata !{metadata !"arg_desc", metadata !12}
!12 = metadata !{i32 1}
!40 = metadata !{metadata !"implicit_arg_desc", metadata !13}
!13 = metadata !{i32 2, metadata !14}
!14 = metadata !{metadata !"explicit_arg_num", i32 4}
!9 = metadata !{metadata !"thread_group_size", i32 1, i32 2, i32 3}
!10 = metadata !{metadata !"thread_group_size_hint", i32 3, i32 2, i32 1}
!22 = metadata !{metadata !"local_size", i32 100}
!23 = metadata !{metadata !"local_offsets", metadata !24}
!24 = metadata !{i32* @globalVar, i32 10}
!29 = metadata !{metadata !"resource_alloc", metadata !30, metadata !31, metadata !32, metadata !33, metadata !36}
!30 = metadata !{metadata !"uavs_num", i32 4}
!31 = metadata !{metadata !"srvs_num", i32 1}
!32 = metadata !{metadata !"samplers_num", i32 7}
!33 = metadata !{metadata !"arg_allocs", metadata !34, metadata !35}
!34 = metadata !{i32 2, i32 0, i32 4}
!35 = metadata !{i32 1, i32 1, i32 2}
!36 = metadata !{metadata !"inline_samplers", metadata !37, metadata !38}
!37 = metadata !{i32 7, i32 1}
!38 = metadata !{i32 4, i32 2}
!39 = metadata !{metadata !"private_memory_per_wi", i32 123}
!11 = metadata !{metadata !"opencl_vec_type_hint", double undef, i1 true}
!15 = metadata !{metadata !"opencl_kernel_arg_addr_space", i32 2, i32 2}
!16 = metadata !{metadata !"opencl_kernel_arg_access_qual", metadata !"none", metadata !"read_write"}
!17 = metadata !{metadata !"opencl_kernel_arg_type", metadata !"char*", metadata !"char*"}
!18 = metadata !{metadata !"opencl_kernel_arg_base_type", metadata !"char*", metadata !"char*"}
!19 = metadata !{metadata !"opencl_kernel_arg_type_qual", metadata !"const", metadata !"restrict"}
!20 = metadata !{metadata !"opencl_kernel_arg_name", metadata !"gooni", metadata !"googoo"}
!5 = metadata !{metadata !"-cl-mad-enable"}
!6 = metadata !{metadata !"-cl-denorms-are-zero"}
!25 = metadata !{metadata !26, i32 6}
!26 = metadata !{i8 4, i8 7}
!27 = metadata !{i32* @globalVar, i32 20}
!28 = metadata !{metadata !45, i32 4}
!45 = metadata !{i8 7, i8 8}
!46 = metadata !{i32 0, i32 20, i32 2, i32 0}
!47 = metadata !{i32 0, i32 40, i32 2, i32 0}

; CHECK: IGC Version
; CHECK-NEXT: 1.1

; CHECK-NEXT: Input IR format
; CHECK-NEXT: ocl 1.1

; CHECK-NEXT: Programming Language version information
; CHECK-NEXT: ocl 1.0
; CHECK-NEXT: ogl 1.1

; CHECK-NEXT: Functions
; CHECK-NEXT: main
; CHECK-NEXT: Function Type: 5

; CHECK-NEXT: Function Arguments Description
; CHECK-NEXT: ID: 1

; CHECK-NEXT: Function Implicit Arguments Description
; CHECK-NEXT: ID: 2
; CHECK-NEXT: Explicit Arg Num: 4

; CHECK-NEXT: Thread Group Size: 1,2,3
; CHECK-NEXT: Thread Group Size Hint: 3,2,1
; CHECK-NEXT: Local Size: 100
; CHECK-NEXT: Local Offsets
; CHECK-NEXT: globalVar: 10
; CHECK-NEXT: Resource Allocation
; CHECK-NEXT: Num UAVs: 4
; CHECK-NEXT: Num SRVs: 1
; CHECK-NEXT: Num Samplers: 7
; CHECK-NEXT: Arg Allocations
; CHECK-NEXT: Type: 2 Extenstion Type: 0 Index: 4
; CHECK-NEXT: Type: 1 Extenstion Type: 1 Index: 2
; CHECK-NEXT: Inline Samplers
; CHECK-NEXT: Value: 7 Index: 1
; CHECK-NEXT: Value: 4 Index: 2
; CHECK-NEXT: Private Memory: 123
; CHECK-NEXT: OpenCL Vector Type Hint: 3, 1
; CHECK-NEXT: OpenCL Arg Address Spaces: 2 2
; CHECK-NEXT: OpenCL Arg Access Qualifiers: none read_write
; CHECK-NEXT: OpenCL Arg Types: char* char*
; CHECK-NEXT: OpenCL Arg Base Types: char* char*
; CHECK-NEXT: OpenCL Arg Type Qualifiers: const restrict
; CHECK-NEXT: OpenCL Arg Names: gooni googoo

; CHECK-NEXT: Compiler Options
; CHECK-NEXT: -cl-mad-enable
; CHECK-NEXT: -cl-denorms-are-zero

; CHECK-NEXT: Inline Constants
; CHECK-NEXT: Buffer: 4 7
; CHECK-NEXT: Alignment: 6

; CHECK-NEXT: Inline Globals
; CHECK-NEXT: Buffer: 7 8
; CHECK-NEXT: Alignment: 4

; CHECK-NEXT: Inline Program Scope Offsets
; CHECK-NEXT: globalVar: 20

; CHECK-NEXT: Global Pointer Program Binary Infos
; CHECK-NEXT: PointerBufferIndex: 0
; CHECK-NEXT: PointerOffset: 20
; CHECK-NEXT: PointeeAddressSpace: 2
; CHECK-NEXT: PointeeBufferIndex: 0

; CHECK-NEXT: Constant Pointer Program Binary Infos
; CHECK-NEXT: PointerBufferIndex: 0
; CHECK-NEXT: PointerOffset: 40
; CHECK-NEXT: PointeeAddressSpace: 2
; CHECK-NEXT: PointeeBufferIndex: 0

; CHECK-NEXT: Floating Point Contractions
; CHECK-NEXT: 1
