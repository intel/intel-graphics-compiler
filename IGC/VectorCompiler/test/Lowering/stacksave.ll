
;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

; CHECK-LABEL: @test_stacksave
define spir_func void @test_stacksave() {
; CHECK: [[FESP:%[^ ]+]] = call i64 @llvm.genx.read.predef.reg.i64.i64(i32 10, i64 undef)
; CHECK: %stack = inttoptr i64 [[FESP]] to i8*
  %stack = call i8* @llvm.stacksave()
; CHECK: [[PTI:%[^ ]+]] = ptrtoint i8* %stack to i64
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[PTI]])
  call void @llvm.stackrestore(i8* %stack)
  ret void
}
