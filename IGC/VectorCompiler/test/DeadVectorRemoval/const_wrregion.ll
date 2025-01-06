;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64>, i64, i32, i32, i32, i16, i32, i1) readnone nounwind willreturn

define <4 x i64> @test1(i64 %val) {
; CHECK: %1 = call <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64> <i64 undef, i64 31, i64 31, i64 7726646165504>, i64 %val, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %1 = call <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64> <i64 0, i64 31, i64 31, i64 7726646165504>, i64 %val, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  ret <4 x i64> %1
}

declare <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8>, <16 x i8>, i32, i32, i32, i16, i32, i1) readnone nounwind willreturn
declare <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8>, i32, i32, i32, i16, i32) readnone nounwind willreturn

define <16 x i8> @test2(<16 x i8> %val) {
; CHECK: %1 = tail call <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8> zeroinitializer, <16 x i8> %val, i32 16, i32 8, i32 2, i16 0, i32 16, i1 true)
  %1 = tail call <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8> zeroinitializer, <16 x i8> %val, i32 16, i32 8, i32 2, i16 0, i32 16, i1 true)
  %.region1 = tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8> %1, i32 0, i32 16, i32 2, i16 1, i32 undef)
  %.region2 = tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8> %1, i32 0, i32 16, i32 2, i16 0, i32 undef)
  %.sum = add <16 x i8> %.region1, %.region2
  ret <16 x i8> %.sum
}

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, <4 x i1>) readnone nounwind willreturn
declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32) readnone nounwind willreturn

define i32 @test3(<4 x i32> %val) {
; CHECK: %1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32> <i32 undef, i32 1, i32 undef, i32 undef>, <4 x i32> %val, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1> <i1 true, i1 false, i1 true, i1 false>)
  %1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> %val, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1> <i1 true, i1 false, i1 true, i1 false>)
  %.region = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %1, i32 0, i32 1, i32 0, i16 4, i32 undef)
  ret i32 %.region
}
