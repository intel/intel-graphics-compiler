;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that we don't dereference nullptr

; REQUIRES: llvm-14-plus
; RUN: igc_opt %s -S -o - -generate-block-mem-ops -platformpvc | FileCheck %s


define spir_kernel void @test() {
; CHECK: @test() {
entry:
  %alloc = alloca i32
  br i1 true, label %BB0, label %BB1

BB0:
  %gep0 = getelementptr i32, i32* %alloc, i32 0
  br label %store_BB

BB1:
  %gep1 = getelementptr i32, i32* %alloc, i32 0
  br label %store_BB

store_BB:
  %store_addr = phi i32* [%gep0, %BB0], [%gep1, %BB1]
  store i32 1, i32* %store_addr
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void ()* @test, !1}
!1 = !{!2, !11}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void ()* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"thread_group_size", i32 32, i32 32, i32 32}
