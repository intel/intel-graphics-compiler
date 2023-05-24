;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare void @llvm.debugtrap()

define spir_func void @test() {
; CHECK: [[CR0:%[^ ]+]] = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
; CHECK: [[CR0_1:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> [[CR0]], i32 0, i32 1, i32 0, i16 4, i32 undef)
; CHECK: [[CR0_1_NEW:%[^ ]+]] = or <1 x i32> [[CR0_1]], <i32 536870912>
; CHECK: [[CR0_NEW:%[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> [[CR0]], <1 x i32> [[CR0_1_NEW]], i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)
; CHECK: call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> [[CR0_NEW]])
  tail call void @llvm.debugtrap()
  ret void
}
