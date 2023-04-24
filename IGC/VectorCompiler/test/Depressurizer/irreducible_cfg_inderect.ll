;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: Sanity checker: nothing should be changed, compilation should not fail

define dllexport void @test(i64 %val) "CMGenxMain" {
  br label %first
first:
  %dst = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %left), i8* blockaddress(@test, %right))
  indirectbr i8* %dst, [label %left, label %right]
right:
  br label %left
left:
  %dst2 = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %right), i8* blockaddress(@test, %exit))
  indirectbr i8* %dst2, [label %right, label %exit]
exit:
  ret void

; CHECK-LABEL: test
; CHECK-NEXT: br label %first
; CHECK-LABEL: first:
; CHECK-NEXT: %dst = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %left), i8* blockaddress(@test, %right))
; CHECK-NEXT: indirectbr i8* %dst, [label %left, label %right]
; CHECK-LABEL: right:
; CHECK-NEXT: br label %left
; CHECK-LABEL: left:
; CHECK-NEXT: %dst2 = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %right), i8* blockaddress(@test, %exit))
; CHECK-NEXT: indirectbr i8* %dst2, [label %right, label %exit]
; CHECK-LABEL: exit:
; CHECK-NEXT: ret void
}

declare i8* @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8, ...)
