;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -genx-simplify -mcpu=Gen9 -march=genx64 -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define <4 x i32> @test_wrr_nontrivial() {
; CHECK-LABEL: @test_wrr_nontrivial(
; CHECK-NEXT:    ret <4 x i32> <i32 0, i32 4, i32 2, i32 4>
 %v = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> <i32 4, i32 4, i32 4, i32 4>, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1>  <i1 false, i1 true, i1 false, i1 true>)
  ret <4 x i32> %v
}

define <4 x i32> @test_wrr_true_vector() {
; CHECK-LABEL: @test_wrr_true_vector(
; CHECK-NEXT:    ret <4 x i32> <i32 4, i32 4, i32 4, i32 4>
 %v = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> <i32 4, i32 4, i32 4, i32 4>, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1>  <i1 true, i1 true, i1 true, i1 true>)
  ret <4 x i32> %v
}

define <4 x i32> @test_wrr_true_scalar() {
; CHECK-LABEL: @test_wrr_true_scalar(
; CHECK-NEXT:    ret <4 x i32> <i32 4, i32 4, i32 4, i32 4>
  %v = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> <i32 4, i32 4, i32 4, i32 4>, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
  ret <4 x i32> %v
}

define <4 x i32> @test_wrr_false_vector() {
; CHECK-LABEL: @test_wrr_false_vector(
; CHECK-NEXT:    ret <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %v = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> <i32 4, i32 4, i32 4, i32 4>, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1>  <i1 false, i1 false, i1 false, i1 false>)
  ret <4 x i32> %v
}

define <4 x i32> @test_wrr_false_scalar() {
; CHECK-LABEL: @test_wrr_false_scalar(
; CHECK-NEXT:    ret <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %v = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> <i32 4, i32 4, i32 4, i32 4>, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 false)
  ret <4 x i32> %v
}

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v4i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, <4 x i1>)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)
