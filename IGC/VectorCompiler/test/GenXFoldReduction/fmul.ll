;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXFoldReduction -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; REQUIRES: llvm_12_or_greater

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"

declare <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float>, i32, i32, i32, i16, i32) #0
declare <64 x float> @llvm.genx.rdregionf.v64f32.v128f32.i16(<128 x float>, i32, i32, i32, i16, i32) #0
declare <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float>, i32, i32, i32, i16, i32) #0
declare <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float>, i32, i32, i32, i16, i32) #0
declare <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float>, i32, i32, i32, i16, i32) #0
declare <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float>, i32, i32, i32, i16, i32) #0
declare <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float>, i32, i32, i32, i16, i32) #0

; CHECK-LABEL: @test_large(
define float @test_large(<256 x float> %src) {
  ; CHECK: %reduce = call reassoc float @llvm.vector.reduce.fmul.v256f32(float 1.000000e+00, <256 x float> %src)
  %1 = call <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float> %src, i32 0, i32 128, i32 1, i16 0, i32 undef)
  %2 = call <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float> %src, i32 0, i32 128, i32 1, i16 512, i32 undef)
  %3 = fmul <128 x float> %1, %2
  %4 = call <64 x float> @llvm.genx.rdregionf.v64f32.v128f32.i16(<128 x float> %3, i32 0, i32 64, i32 1, i16 0, i32 undef)
  %5 = call <64 x float> @llvm.genx.rdregionf.v64f32.v128f32.i16(<128 x float> %3, i32 0, i32 64, i32 1, i16 256, i32 undef)
  %6 = fmul <64 x float> %4, %5
  %7 = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %6, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %8 = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %6, i32 0, i32 32, i32 1, i16 128, i32 undef)
  %9 = fmul <32 x float> %7, %8
  %10 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %9, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %11 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %9, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %12 = fmul <16 x float> %10, %11
  %13 = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> %12, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %14 = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> %12, i32 0, i32 8, i32 1, i16 32, i32 undef)
  %15 = fmul <8 x float> %13, %14
  %16 = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> %15, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %17 = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> %15, i32 0, i32 4, i32 1, i16 16, i32 undef)
  %18 = fmul <4 x float> %16, %17
  %19 = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> %18, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %20 = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> %18, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %21 = fmul <2 x float> %19, %20
  %22 = extractelement <2 x float> %21, i32 0
  %23 = extractelement <2 x float> %21, i32 1
  %reduce = fmul float %22, %23
  ret float %reduce
}

attributes #0 = { nofree nosync nounwind readnone willreturn }
