;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.genx.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32>, <16 x i16>)
declare <16 x i32> @llvm.genx.packed.4bit.upconvert.lut.v16i32.v16i8(<16 x i32>, <16 x i8>)
declare <32 x i32> @llvm.genx.packed.4bit.upconvert.lut.v32i32.v32i8(<16 x i32>, <32 x i8>)

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)
declare <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32)
declare <32 x i8> @llvm.genx.rdregioni.v32i8.v128i8.i16(<128 x i8>, i32, i32, i32, i16, i32)

; CHECK-LABEL: test_v16i16
define <32 x i32> @test_v16i16(<16 x i32> %lut, <32 x i16> %src.i16) {
  %src.i16.0 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %src.i16, i32 2, i32 1, i32 0, i16 0, i32 undef)
  %src.i16.1 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %src.i16, i32 2, i32 1, i32 0, i16 2, i32 undef)

  ; CHECK: %res.0 = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.i16.0)
  ; CHECK: %res.1 = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.i16.1)
  %res.0 = call <16 x i32> @llvm.genx.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.i16.0)
  %res.1 = call <16 x i32> @llvm.genx.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.i16.1)

  %res = shufflevector <16 x i32> %res.0, <16 x i32> %res.1, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>

  ret <32 x i32> %res
}

; CHECK-LABEL: @test_v16i8
define <16 x i32> @test_v16i8(<16 x i32> %lut, <64 x i8> %src.i8) {
  %src.i8.3 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %src.i8, i32 4, i32 1, i32 0, i16 3, i32 undef)

  ; CHECK: %res.3 = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i8(<16 x i32> %lut, <16 x i8> %src.i8.3)
  %res.3 = call <16 x i32> @llvm.genx.packed.4bit.upconvert.lut.v16i32.v16i8(<16 x i32> %lut, <16 x i8> %src.i8.3)

  ret <16 x i32> %res.3
}

; CHECK-LABEL: @test_v32i8
define <32 x i32> @test_v32i8(<16 x i32> %lut, <128 x i8> %src.i8) {
  %src.i8.3 = call <32 x i8> @llvm.genx.rdregioni.v32i8.v128i8.i16(<128 x i8> %src.i8, i32 4, i32 1, i32 0, i16 3, i32 undef)

  ; CHECK: [[LUT:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v16i32.i16(<16 x i32> %lut, i32 0, i32 16, i32 1, i16 0, i32 0)
  ; CHECK: %res.3 = call <32 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v32i32.v32i8(<32 x i32> [[LUT]], <32 x i8> %src.i8.3)
  %res.3 = call <32 x i32> @llvm.genx.packed.4bit.upconvert.lut.v32i32.v32i8(<16 x i32> %lut, <32 x i8> %src.i8.3)

  ret <32 x i32> %res.3
}
