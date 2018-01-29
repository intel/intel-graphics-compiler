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
; RUN: opt -igc-programscope-constant-resolve -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ProgramScopeConstantResolution pass handles
;; line & variable debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

@x = addrspace(2) constant [2 x float] [float 2.000000e+00, float 3.000000e+00], align 4

; Function Attrs: alwaysinline nounwind
define void @test(float addrspace(1)* %dst, i32 %i, i8 addrspace(2)* %constBase) #0 {
entry:
  br label %BB
BB:
  %arrayidx = getelementptr inbounds [2 x float] addrspace(2)* @x, i32 0, i32 %i, !dbg !12
  %res = load float addrspace(2)* %arrayidx, align 4, !dbg !13
  store float %res, float addrspace(1)* %dst, align 4
  ret void

; CHECK: entry:
; CHECK: [[offx:%[a-zA-Z0-9]+]] = getelementptr i8 addrspace(2)* %constBase, i32 0
; CHECK: [[castx:%[a-zA-Z0-9]+]] = bitcast i8 addrspace(2)* [[offx]] to [2 x float] addrspace(2)*
; CHECK: call void @llvm.dbg.declare(metadata !{[2 x float] addrspace(2)* [[castx]]}, metadata [[m20:![a-zA-Z0-9]+]])
; CHECK: br label %BB

; CHECK: BB:
; CHECK: [[arrayidx:%[a-zA-Z0-9]+]] = getelementptr inbounds [2 x float] addrspace(2)* [[castx]], i32 0, i32 %i, !dbg !12
; CHECK: ret void
}

; CHECK: [[m20]] = metadata !{i32 786688, metadata !3, metadata !"x", metadata !5, i32 2, metadata !8, i32 0, i32 0}

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19}

!llvm.dbg.cu = !{!0}
!igc.functions = !{!18}
!igc.inline.programscope.offsets = !{!19}

!0 = metadata !{i32 786449, metadata !2, i32 12, metadata !"clang version 3.4 ", i1 false, metadata !"", i32 0, null, null, metadata !3, metadata !6, null, metadata !""} ; [ DW_TAG_compile_unit ] [dirname/filename.cl] [DW_LANG_C99]
!1 = metadata !{metadata !"filename.cl", metadata !"dirname"}
!2 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, null, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"test", metadata !"test", metadata !"", i32 3, metadata !2, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, null, null, null, null, i32 4} ; [ DW_TAG_subprogram ] [line 3] [def] [scope 4] [test]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [dirname/filename.cl]
!6 = metadata !{metadata !7}
!7 = metadata !{i32 786484, i32 0, null, metadata !"x", metadata !"x", metadata !"", metadata !5, i32 2, metadata !8, i32 0, i32 1, [2 x float] addrspace(2)* @x, null} ; [ DW_TAG_variable ] [x] [line 165] [def]
!8 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 64, i64 32, i32 0, i32 0, metadata !9, metadata !10, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 64, align 32, offset 0] [from ]
!9 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from float]
!10 = metadata !{metadata !11}
!11 = metadata !{i32 786465, i64 0, i64 2}        ; [ DW_TAG_subrange_type ] [0, 1]
!12 = metadata !{i32 5, i32 0, metadata !4, null}
!13 = metadata !{i32 6, i32 0, metadata !4, null}
!14 = metadata !{metadata !"function_type", i32 0}
!15 = metadata !{i32 10} ;; CONSTANT_BASE
!16 = metadata !{metadata !"implicit_arg_desc", metadata !15}
!17 = metadata !{metadata !14, metadata !16}
!18 = metadata !{void (float addrspace(1)*, i32, i8 addrspace(2)*)* @test, metadata !17}
!19 = metadata !{[2 x float] addrspace(2)* @x, i32 0}

