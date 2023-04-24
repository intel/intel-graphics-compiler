;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -mattr=+emulate_i64 -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8>, i32, i32, i32, i16, i32)
declare <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8>, <16 x i8>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @byte_test_1
define <16 x i8> @byte_test_1(<32 x i8> %val) {
; CHECK: %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %1 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8> %val, i32 0, i32 16, i32 1, i16 0, i32 undef)
  ret <16 x i8> %1
}

; CHECK-LABEL: @byte_test_2
define <32 x i8> @byte_test_2(<32 x i8> %val) {
; CHECK: %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %1 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v32i8.i16(<32 x i8> %val, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: %2 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> %{{[a-zA-Z0-9_.]*}}, <4 x i32> %1, i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
  %2 = call <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8> %val, <16 x i8> %1, i32 0, i32 16, i32 1, i16 16, i32 undef, i1 true)
  ret <32 x i8> %2
}

declare <2 x i64> @llvm.genx.rdregioni.v2i64.v4i64.i16(<4 x i64>, i32, i32, i32, i16, i32)
declare <4 x i64> @llvm.genx.wrregioni.v4i64.v2i64.i16.i1(<4 x i64>, <2 x i64>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @qword_test_1
define <2 x i64> @qword_test_1(<4 x i64> %val) {
; CHECK: %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %1 = call <2 x i64> @llvm.genx.rdregioni.v2i64.v4i64.i16(<4 x i64> %val, i32 0, i32 2, i32 1, i16 0, i32 undef)
  ret <2 x i64> %1
}

; CHECK-LABEL: @qword_test_2
define <4 x i64> @qword_test_2(<4 x i64> %val) {
; CHECK: %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %1 = call <2 x i64> @llvm.genx.rdregioni.v2i64.v4i64.i16(<4 x i64> %val, i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK: %2 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> %{{[a-zA-Z0-9_.]*}}, <4 x i32> %1, i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
  %2 = call <4 x i64> @llvm.genx.wrregioni.v4i64.v2i64.i16.i1(<4 x i64> %val, <2 x i64> %1, i32 0, i32 2, i32 1, i16 16, i32 undef, i1 true)
  ret <4 x i64> %2
}

declare <4 x i64> @llvm.genx.rdregioni.v4i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i64> @llvm.genx.wrregioni.v8i64.v4i64.i16.i1(<8 x i64>, <4 x i64>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_2d_region
define <8 x i64> @test_2d_region(<8 x i64> %val) {
; CHECK: %1 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %{{[a-zA-Z0-9_.]*}}, i32 4, i32 2, i32 1, i16 8, i32 undef)
  %1 = call <4 x i64> @llvm.genx.rdregioni.v4i64.v8i64.i16(<8 x i64> %val, i32 2, i32 1, i32 1, i16 8, i32 undef)
; CHECK: %2 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %{{[a-zA-Z0-9_.]*}}, <8 x i32> %1, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %2 = call <8 x i64> @llvm.genx.wrregioni.v8i64.v4i64.i16.i1(<8 x i64> %val, <4 x i64> %1, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
  ret <8 x i64> %2
}

declare <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64>, <16 x i64>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @qword_test_big_vector
; CHECK-NEXT:  %val.cast = bitcast <32 x i64> %val to <64 x i32>
; CHECK-NEXT:  %val.cast1 = bitcast <32 x i64> %val to <64 x i32>
; CHECK-NEXT:  %.split0 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val.cast, i32 16, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %.join0 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %val.cast1, <16 x i32> %.split0, i32 16, i32 16, i32 1, i16 128, i32 undef, i1 true)
; CHECK-NEXT:  %.split16 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val.cast, i32 16, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  %.join16 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.join0, <16 x i32> %.split16, i32 16, i32 16, i32 1, i16 192, i32 undef, i1 true)
define <32 x i64> @qword_test_big_vector(<32 x i64> %val) {
  %1 = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %val, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = call <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64> %val, <16 x i64> %1, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true)
  ret <32 x i64> %2
}
