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
; RUN: opt -igc-agg-arg -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ResolveAggregateArguments pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

%struct.AA = type { <4 x float>, i32, [3 x i64] }

define void @test(float addrspace(1)* %dst, %struct.AA* byval %src, float %const_reg_fp32, float %const_reg_fp321, float %const_reg_fp322, float %const_reg_fp323, i32 %const_reg_dword, i64 %const_reg_qword, i64 %const_reg_qword4, i64 %const_reg_qword5) #0 {
entry:
  call void @llvm.dbg.value(metadata !{float addrspace(1)* %dst}, i64 0, metadata !4), !dbg !1
  call void @llvm.dbg.declare(metadata !{%struct.AA* %src}, metadata !5), !dbg !1
  %a = getelementptr inbounds %struct.AA* %src, i32 0, i32 0, !dbg !2
  %0 = load <4 x float>* %a, align 16, !dbg !2
  %1 = extractelement <4 x float> %0, i32 0, !dbg !2
  store float %1, float addrspace(1)* %dst, align 4, !dbg !2
  ret void, !dbg !3

; CHECK: [[src_alloca:%[a-zA-Z0-9_]+]] = alloca %struct.AA
; CHECK: call void @llvm.dbg.value(metadata !{float addrspace(1)* %dst}, i64 0, metadata !4), !dbg !1
; CHECK: call void @llvm.dbg.declare(metadata !{%struct.AA* [[src_alloca]]}, metadata !5), !dbg !1
; CHECK: [[a:%[a-zA-Z0-9_]+]] = getelementptr inbounds %struct.AA* [[src_alloca]], i32 0, i32 0, !dbg !2
; CHECK: [[inst1:%[a-zA-Z0-9_]+]] = load <4 x float>* [[a]], align 16, !dbg !2
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

!0 = metadata !{}
!1 = metadata !{i32 6, i32 0, metadata !0, null}
!2 = metadata !{i32 8, i32 0, metadata !0, null}
!3 = metadata !{i32 9, i32 0, metadata !0, null}
!4 = metadata !{i32 786689, metadata !0, metadata !"dst", metadata !0, i32 16777222, metadata !0, i32 0, i32 0}
!5 = metadata !{i32 786689, metadata !0, metadata !"src", metadata !0, i32 33554438, metadata !0, i32 0, i32 0}
!6 = metadata !{metadata !"function_type", i32 0}
!7 = metadata !{metadata !"explicit_arg_num", i32 1}
!8 = metadata !{metadata !"struct_arg_offset", i32 0}
!9 = metadata !{i32 14, metadata !7, metadata !8}   ;; CONSTANT_REG_FP32
!10 = metadata !{metadata !"struct_arg_offset", i32 4}
!11 = metadata !{i32 14, metadata !7, metadata !10} ;; CONSTANT_REG_FP32
!12 = metadata !{metadata !"struct_arg_offset", i32 8}
!13 = metadata !{i32 14, metadata !7, metadata !12} ;; CONSTANT_REG_FP32
!14 = metadata !{metadata !"struct_arg_offset", i32 12}
!15 = metadata !{i32 14, metadata !7, metadata !14} ;; CONSTANT_REG_FP32
!16 = metadata !{metadata !"struct_arg_offset", i32 16}
!17 = metadata !{i32 16, metadata !7, metadata !16} ;; CONSTANT_REG_DWORD
!18 = metadata !{metadata !"struct_arg_offset", i32 24}
!19 = metadata !{i32 15, metadata !7, metadata !18} ;; CONSTANT_REG_QWORD
!20 = metadata !{metadata !"struct_arg_offset", i32 32}
!21 = metadata !{i32 15, metadata !7, metadata !20} ;; CONSTANT_REG_QWORD
!22 = metadata !{metadata !"struct_arg_offset", i32 40}
!23 = metadata !{i32 15, metadata !7, metadata !22} ;; CONSTANT_REG_QWORD
!24 = metadata !{metadata !"implicit_arg_desc", metadata !9, metadata !11, metadata !13, metadata !15, metadata !17, metadata !19, metadata !21, metadata !23}
!25 = metadata !{metadata !6, metadata !24}
!26 = metadata !{void (float addrspace(1)*, %struct.AA*, float, float, float, float, i32, i64, i64, i64)* @test, metadata !25}

