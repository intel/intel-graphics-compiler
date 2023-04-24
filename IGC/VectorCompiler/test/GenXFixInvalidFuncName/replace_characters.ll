;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXFixInvalidFuncName -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s

; COM: This test checks whether GenXFixInvalidFuncName replaces
; COM: $ and . with _ in SPIR function names.

; CHECK-LABEL: define spir_func void @test_spir_func_caller(
; CHECK-NEXT: call spir_func void @test__spir_func_callee()
; CHECK-NEXT: call void @"test$.callee"()
define spir_func void @test$spir.func_caller() #0 {
  call spir_func void @test$.spir.func_callee()
  call void @test$.callee()
  ret void
}

; CHECK-LABEL: define spir_func void @test__spir_func_callee(
define spir_func void @test$.spir.func_callee() #0 {
  ret void
}

; CHECK-LABEL: define void @"test$.callee"(
define void @test$.callee() {
  ret void
}

attributes #0 = { nounwind "CMStackCall" }