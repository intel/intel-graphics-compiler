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
; RUN: igc_opt -igc-agg-arg -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ResolveAggregateArguments pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

%struct.AA = type { <4 x float>, i32, [3 x i64] }

define void @test(float addrspace(1)* %dst, %struct.AA* byval %src, float %const_reg_fp32, float %const_reg_fp321, float %const_reg_fp322, float %const_reg_fp323, i32 %const_reg_dword, i64 %const_reg_qword, i64 %const_reg_qword4, i64 %const_reg_qword5) #0 {
entry:
  call void @llvm.dbg.value(metadata float addrspace(1)* %dst, i64 0, metadata !4), !dbg !1
  call void @llvm.dbg.declare(metadata %struct.AA* %src, metadata !5), !dbg !1
  %a = getelementptr inbounds %struct.AA, %struct.AA* %src, i32 0, i32 0, !dbg !2
  %0 = load <4 x float>, <4 x float>* %a, align 16, !dbg !2
  %1 = extractelement <4 x float> %0, i32 0, !dbg !2
  store float %1, float addrspace(1)* %dst, align 4, !dbg !2
  ret void, !dbg !3

; CHECK: [[src_alloca:%[a-zA-Z0-9_]+]] = alloca %struct.AA
; CHECK: call void @llvm.dbg.value(metadata !{float addrspace(1)* %dst}, i64 0, metadata !4), !dbg !1
; CHECK: call void @llvm.dbg.declare(metadata !{%struct.AA* [[src_alloca]]}, metadata !5), !dbg !1
; CHECK: [[a:%[a-zA-Z0-9_]+]] = getelementptr inbounds %struct.AA, %struct.AA* [[src_alloca]], i32 0, i32 0, !dbg !2
; CHECK: [[inst1:%[a-zA-Z0-9_]+]] = load <4 x float>, <4 x float>* [[a]], align 16, !dbg !2
; CHECK: [[inst2:%[a-zA-Z0-9_]+]] = extractelement <4 x float> [[inst1]], i32 0, !dbg !2
; CHECK: store float [[inst2]], float addrspace(1)* %dst, align 4, !dbg !2
; CHECK: ret void, !dbg !3
}

declare void @llvm.dbg.declare(metadata, metadata) #1

declare void @llvm.dbg.value(metadata, i64, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26}

!igc.functions = !{!26}

!0 = !{}
!1 = !{i32 6, i32 0, !0, null}
!2 = !{i32 8, i32 0, !0, null}
!3 = !{i32 9, i32 0, !0, null}
!4 = !{i32 786689, !0, !"dst", !0, i32 16777222, !0, i32 0, i32 0}
!5 = !{i32 786689, !0, !"src", !0, i32 33554438, !0, i32 0, i32 0}
!6 = !{!"function_type", i32 0}
!7 = !{!"explicit_arg_num", i32 1}
!8 = !{!"struct_arg_offset", i32 0}
!9 = !{i32 14, !7, !8}   ;; CONSTANT_REG_FP32
!10 = !{!"struct_arg_offset", i32 4}
!11 = !{i32 14, !7, !10} ;; CONSTANT_REG_FP32
!12 = !{!"struct_arg_offset", i32 8}
!13 = !{i32 14, !7, !12} ;; CONSTANT_REG_FP32
!14 = !{!"struct_arg_offset", i32 12}
!15 = !{i32 14, !7, !14} ;; CONSTANT_REG_FP32
!16 = !{!"struct_arg_offset", i32 16}
!17 = !{i32 16, !7, !16} ;; CONSTANT_REG_DWORD
!18 = !{!"struct_arg_offset", i32 24}
!19 = !{i32 15, !7, !18} ;; CONSTANT_REG_QWORD
!20 = !{!"struct_arg_offset", i32 32}
!21 = !{i32 15, !7, !20} ;; CONSTANT_REG_QWORD
!22 = !{!"struct_arg_offset", i32 40}
!23 = !{i32 15, !7, !22} ;; CONSTANT_REG_QWORD
!24 = !{!"implicit_arg_desc", !9, !11, !13, !15, !17, !19, !21, !23}
!25 = !{!6, !24}
!26 = !{void (float addrspace(1)*, %struct.AA*, float, float, float, float, i32, i64, i64, i64)* @test, !25}

