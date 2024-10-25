;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

; CHECK-LABEL: @test_stacksave
define spir_func void @test_stacksave() {
; CHECK: [[FESP:%[^ ]+]] = call i64 @llvm.genx.read.predef.reg.i64.i64(i32 10, i64 undef)
; CHECK-TYPED-PTRS: %stack = inttoptr i64 [[FESP]] to i8*
; CHECK-OPAQUE-PTRS: %stack = inttoptr i64 [[FESP]] to ptr
  %stack = call i8* @llvm.stacksave()
; CHECK-TYPED-PTRS: [[PTI:%[^ ]+]] = ptrtoint i8* %stack to i64
; CHECK-OPAQUE-PTRS: [[PTI:%[^ ]+]] = ptrtoint ptr %stack to i64
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[PTI]])
  call void @llvm.stackrestore(i8* %stack)
  ret void
}
