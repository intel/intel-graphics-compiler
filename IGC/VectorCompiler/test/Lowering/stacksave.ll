
;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

; CHECK-LABEL: @test_stacksave
define spir_func void @test_stacksave() {
; CHECK: [[READ1:[^ ]*]] = call i64 @llvm.genx.read.predef.reg.i64.i64(i32 10, i64 undef)
; CHECK: [[READ2:[^ ]*]] = call i64 @llvm.genx.rdregioni.i64.i64.i16(i64 [[READ1]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: [[STACK:[^ ]*]] = inttoptr i64 %2 to i8*
  %stack = call i8* @llvm.stacksave()
; CHECK: [[WRITE1:[^ ]*]] = ptrtoint i8* [[STACK]] to i64
; CHECK: [[WRITE2:[^ ]*]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 [[WRITE1]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 [[WRITE2]])
  call void @llvm.stackrestore(i8* %stack)
  ret void
}

