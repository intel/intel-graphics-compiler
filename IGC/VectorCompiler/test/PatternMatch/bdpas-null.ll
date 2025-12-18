;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float>, <128 x i32>, <64 x i32>, <16 x i8>, <8 x i8>, i32, i32, i32, i32)

; CHECK-LABEL: @test1(
define <128 x float> @test1(<128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale) {
; CHECK: %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> undef, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
  %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> zeroinitializer, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
  ret <128 x float> %res
}

; CHECK-LABEL: @test2(
define <128 x float> @test2(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <8 x i8> %ascale) {
; CHECK: %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> undef, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
  %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
  ret <128 x float> %res
}

; CHECK-LABEL: @test3(
define <128 x float> @test3(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale) {
; CHECK: %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> undef, i32 9, i32 9, i32 8, i32 8)
  %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 9, i32 9, i32 8, i32 8)
  ret <128 x float> %res
}

; CHECK-LABEL: @test4(
define <128 x float> @test4(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a) {
; CHECK: %res = call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, i32 9, i32 9, i32 8, i32 8, i32 0, i32 0)
  %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, <8 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 9, i32 9, i32 8, i32 8)
  ret <128 x float> %res
}
