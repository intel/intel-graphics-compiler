;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that we don't dereference nullptr

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -generate-block-mem-ops -platformpvc | FileCheck %s


define spir_kernel void @test() {
; CHECK: @test() {
entry:
  %alloc = alloca i32, align 4
  br i1 true, label %BB0, label %BB1

BB0:                                              ; preds = %entry
  %gep0 = getelementptr i32, ptr %alloc, i32 0
  br label %store_BB

BB1:                                              ; preds = %entry
  %gep1 = getelementptr i32, ptr %alloc, i32 0
  br label %store_BB

store_BB:                                         ; preds = %BB1, %BB0
  %store_addr = phi ptr [ %gep0, %BB0 ], [ %gep1, %BB1 ]
  store i32 1, ptr %store_addr, align 4
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"thread_group_size", i32 32, i32 32, i32 32}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", ptr @test}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"workGroupWalkOrder", !9, !10, !11}
!9 = !{!"dim0", i32 0}
!10 = !{!"dim1", i32 1}
!11 = !{!"dim2", i32 2}
