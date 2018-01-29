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
; RUN: opt -igc-add-implicit-gid -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ImplicitGID pass handles generates the following:
;;   1. calls to get_global_id(x), where x = 0, 1, 2
;;   2. debug info metadata for variables with these names:
;;      __ocl_dbg_gid0, __ocl_dbg_gid1, __ocl_dbg_gid2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN9"

; Function Attrs: nounwind
define void @foo() #0 {
entry:
  ret void, !dbg !11

; CHECK: [[__ocl_dbg_gid0:%[a-zA-Z0-9_.]+]] = alloca i64
; CHECK: call void @llvm.dbg.declare(metadata !{i64* [[__ocl_dbg_gid0]]}, metadata [[m12:![0-9]+]]), !dbg !11
; CHECK: [[globalId0:%[a-zA-Z0-9_.]+]] = call i32 @_Z13get_global_idj(i32 0)
; CHECK: [[gid0_i64:%[a-zA-Z0-9_.]+]] = zext i32 [[globalId0]] to i64
; CHECK: store i64 [[gid0_i64]], i64* [[__ocl_dbg_gid0]]
; CHECK: [[__ocl_dbg_gid1:%[a-zA-Z0-9_.]+]] = alloca i64
; CHECK: call void @llvm.dbg.declare(metadata !{i64* %__ocl_dbg_gid1}, metadata [[m14:![0-9]+]]), !dbg !11
; CHECK: [[globalId1:%[a-zA-Z0-9_.]+]] = call i32 @_Z13get_global_idj(i32 1)
; CHECK: [[gid1_i64:%[a-zA-Z0-9_.]+]] = zext i32 [[globalId1]] to i64
; CHECK: store i64 [[gid1_i64]], i64* [[__ocl_dbg_gid1]]
; CHECK: [[__ocl_dbg_gid2:%[a-zA-Z0-9_.]+]] = alloca i64
; CHECK: call void @llvm.dbg.declare(metadata !{i64* [[__ocl_dbg_gid2]]}, metadata [[m15:![0-9]+]]), !dbg !11
; CHECK: [[globalId2:%[a-zA-Z0-9_.]+]] = call i32 @_Z13get_global_idj(i32 2)
; CHECK: [[gid2_i64:%[a-zA-Z0-9_.]+]] = zext i32 [[globalId2]] to i64
; CHECK: store i64 [[gid2_i64]], i64* [[__ocl_dbg_gid2]]
; CHECK: ret void, !dbg !11
}

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11}

!llvm.dbg.cu = !{!0}
!igc.functions = !{!8}

!0 = metadata !{i32 786449, metadata !1, i32 12, metadata !"clang version 3.4 ", i1 false, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !""} ; [ DW_TAG_compile_unit ] [dir/path] [DW_LANG_C99]
!1 = metadata !{metadata !"filname", metadata !"dir"}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"foo", metadata !"foo", metadata !"", i32 1, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 0, i1 false, void ()* @foo, null, null, metadata !2, i32 2} ; [ DW_TAG_subprogram ] [line 1] [def] [scope 2] [foo]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [dir/filename]
!6 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{null}
!8 = metadata !{void ()* @foo, metadata !9}
!9 = metadata !{metadata !10}
!10 = metadata !{metadata !"function_type", i32 0}
!11 = metadata !{i32 3, i32 0, metadata !4, null}

; CHECK: [[m12]] = metadata !{i32 786688, metadata !4, metadata !"__ocl_dbg_gid0", null, i32 1, metadata [[m13:![0-9]+]], i32 64, i32 0}
; CHECK: [[m13]] = metadata !{i32 786468, null, null, metadata !"long unsigned int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 7}
; CHECK: [[m14]] = metadata !{i32 786688, metadata !4, metadata !"__ocl_dbg_gid1", null, i32 1, metadata [[m13]], i32 64, i32 0}
; CHECK: [[m15]] = metadata !{i32 786688, metadata !4, metadata !"__ocl_dbg_gid2", null, i32 1, metadata [[m13]], i32 64, i32 0}
