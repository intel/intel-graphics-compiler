;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32, i64) nounwind readonly
declare <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32) nounwind readnone
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>) nounwind

; CHECK: bales in function: test
; CHECK: call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 1, i32 1, i16 0, i32 undef): rdregion
define void @test() {
  %1 = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
  %2 = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %1, i32 0, i32 1, i32 1, i16 0, i32 undef)
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %2, <1 x i32> zeroinitializer)
  ret void
}
