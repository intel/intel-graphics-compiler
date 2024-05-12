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

declare <128 x float> @llvm.maxnum.v128f32(<128 x float>, <128 x float>)
declare <64 x float> @llvm.maxnum.v64f32(<64 x float>, <64 x float>)
declare <32 x float> @llvm.maxnum.v32f32(<32 x float>, <32 x float>)
declare <16 x float> @llvm.maxnum.v16f32(<16 x float>, <16 x float>)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>)
declare <4 x float> @llvm.maxnum.v4f32(<4 x float>, <4 x float>)
declare <2 x float> @llvm.maxnum.v2f32(<2 x float>, <2 x float>)
declare float @llvm.maxnum.f32(float, float)

; CHECK-LABEL: @test_large(
define float @test_large(<256 x float> %src) {
  ; CHECK: %reduce = call reassoc float @llvm.vector.reduce.fmax.v256f32(<256 x float> %src)
  %1 = call <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float> %src, i32 0, i32 128, i32 1, i16 0, i32 undef)
  %2 = call <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float> %src, i32 0, i32 128, i32 1, i16 512, i32 undef)
  %3 = call <128 x float> @llvm.maxnum.v128f32(<128 x float> %1, <128 x float> %2)
  %4 = call <64 x float> @llvm.genx.rdregionf.v64f32.v128f32.i16(<128 x float> %3, i32 0, i32 64, i32 1, i16 0, i32 undef)
  %5 = call <64 x float> @llvm.genx.rdregionf.v64f32.v128f32.i16(<128 x float> %3, i32 0, i32 64, i32 1, i16 256, i32 undef)
  %6 = call <64 x float> @llvm.maxnum.v64f32(<64 x float> %4, <64 x float> %5)
  %7 = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %6, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %8 = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> %6, i32 0, i32 32, i32 1, i16 128, i32 undef)
  %9 = call <32 x float> @llvm.maxnum.v32f32(<32 x float> %7, <32 x float> %8)
  %10 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %9, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %11 = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %9, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %12 = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %10, <16 x float> %11)
  %13 = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> %12, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %14 = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> %12, i32 0, i32 8, i32 1, i16 32, i32 undef)
  %15 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %13, <8 x float> %14)
  %16 = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> %15, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %17 = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> %15, i32 0, i32 4, i32 1, i16 16, i32 undef)
  %18 = call <4 x float> @llvm.maxnum.v4f32(<4 x float> %16, <4 x float> %17)
  %19 = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> %18, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %20 = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> %18, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %21 = call <2 x float> @llvm.maxnum.v2f32(<2 x float> %19, <2 x float> %20)
  %22 = extractelement <2 x float> %21, i32 0
  %23 = extractelement <2 x float> %21, i32 1
  %reduce = call float @llvm.maxnum.f32(float %22, float %23)
  ret float %reduce
}

attributes #0 = { nofree nosync nounwind readnone willreturn }
