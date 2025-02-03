;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
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
; CHECK-TYPED-PTRS-NEXT: %dst = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %left), i8* blockaddress(@test, %right))
; CHECK-TYPED-PTRS-NEXT: indirectbr i8* %dst, [label %left, label %right]
; CHECK-OPAQUE-PTRS-NEXT: %dst = call ptr (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, ptr blockaddress(@test, %left), ptr blockaddress(@test, %right))
; CHECK-OPAQUE-PTRS-NEXT: indirectbr ptr %dst, [label %left, label %right]
; CHECK-LABEL: right:
; CHECK-NEXT: br label %left
; CHECK-LABEL: left:
; CHECK-TYPED-PTRS-NEXT: %dst2 = call i8* (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, i8* blockaddress(@test, %right), i8* blockaddress(@test, %exit))
; CHECK-TYPED-PTRS-NEXT: indirectbr i8* %dst2, [label %right, label %exit]
; CHECK-OPAQUE-PTRS-NEXT: %dst2 = call ptr (i8, ...) @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8 0, ptr blockaddress(@test, %right), ptr blockaddress(@test, %exit))
; CHECK-OPAQUE-PTRS-NEXT: indirectbr ptr %dst2, [label %right, label %exit]
; CHECK-LABEL: exit:
; CHECK-NEXT: ret void
}

declare i8* @llvm.genx.jump.table.p0i8.i8.p0i8.p0i8(i8, ...)
