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
; RUN: igc_opt -igc-add-implicit-gid -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ImplicitGID pass handles generates the following:
;;   1. calls to get_global_id(x), where x = 0, 1, 2
;;   2. debug info metadata for variables with these names:
;;      __ocl_dbg_gid0, __ocl_dbg_gid1, __ocl_dbg_gid2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

!0 = !{i32 786449, !1, i32 12, !"clang version 3.4 ", i1 false, !"", i32 0, !2, !2, !3, !2, !2, !""} ; [ DW_TAG_compile_unit ] [dir/path] [DW_LANG_C99]
!1 = !{!"filname", !"dir"}
!2 = !{i32 0}
!3 = !{!4}
!4 = !{i32 786478, !1, !5, !"foo", !"foo", !"", i32 1, !6, i1 false, i1 true, i32 0, i32 0, null, i32 0, i1 false, void ()* @foo, null, null, !2, i32 2} ; [ DW_TAG_subprogram ] [line 1] [def] [scope 2] [foo]
!5 = !{i32 786473, !1}          ; [ DW_TAG_file_type ] [dir/filename]
!6 = !{i32 786453, i32 0, i32 0, !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, !7, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = !{null}
!8 = !{void ()* @foo, !9}
!9 = !{!10}
!10 = !{!"function_type", i32 0}
!11 = !{i32 3, i32 0, !4, null}

; CHECK: [[m12]] = !{i32 786688, !4, !"__ocl_dbg_gid0", null, i32 1, [[m13:![0-9]+]], i32 64, i32 0}
; CHECK: [[m13]] = !{i32 786468, null, null, !"long unsigned int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 7}
; CHECK: [[m14]] = !{i32 786688, !4, !"__ocl_dbg_gid1", null, i32 1, [[m13]], i32 64, i32 0}
; CHECK: [[m15]] = !{i32 786688, !4, !"__ocl_dbg_gid2", null, i32 1, [[m13]], i32 64, i32 0}
