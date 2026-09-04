;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-process-bi-code-assumption -S < %s 2>&1 | FileCheck %s

; Indirect calls and inline asm have no called function (getCalledFunction()
; returns nullptr). matchBuiltin must not dereference it. These calls are not
; builtins, so the code must be left unchanged (no trunc/zext inserted).

; CHECK-LABEL: @test_inline_asm
; CHECK-NEXT:  %1 = call i64 asm "", "=r"()
; CHECK-NEXT:  %2 = icmp ult i64 %1, 2147483648
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define spir_kernel void @test_inline_asm() {
  %1 = call i64 asm "", "=r"()
  %2 = icmp ult i64 %1, 2147483648
  call void @llvm.assume(i1 %2)
  ret void
}

; CHECK-LABEL: @test_indirect_call
; CHECK-NEXT:  %1 = call spir_func i64 null(i32 0)
; CHECK-NEXT:  %2 = icmp ult i64 %1, 0
; CHECK-NEXT:  call void @llvm.assume(i1 %2)
; CHECK-NEXT:  ret void
define spir_kernel void @test_indirect_call() {
  %1 = call spir_func i64 null(i32 0)
  %2 = icmp ult i64 %1, 0
  call void @llvm.assume(i1 %2)
  ret void
}

declare void @llvm.assume(i1)
