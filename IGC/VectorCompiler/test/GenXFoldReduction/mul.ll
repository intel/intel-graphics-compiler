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

declare <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32>, i32, i32, i32, i16, i32) #0
declare <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32>, i32, i32, i32, i16, i32) #0
declare <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32) #0
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32) #0
declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32) #0
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #0
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32) #0

; CHECK-LABEL: @test_large(
define i32 @test_large(<256 x i32> %src) {
  ; CHECK: %reduce = call i32 @llvm.vector.reduce.mul.v256i32(<256 x i32> %src)
  %1 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src, i32 0, i32 128, i32 1, i16 0, i32 undef)
  %2 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src, i32 0, i32 128, i32 1, i16 512, i32 undef)
  %3 = mul <128 x i32> %1, %2
  %4 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %3, i32 0, i32 64, i32 1, i16 0, i32 undef)
  %5 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %3, i32 0, i32 64, i32 1, i16 256, i32 undef)
  %6 = mul <64 x i32> %4, %5
  %7 = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %6, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %8 = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %6, i32 0, i32 32, i32 1, i16 128, i32 undef)
  %9 = mul <32 x i32> %7, %8
  %10 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %9, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %11 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %9, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %12 = mul <16 x i32> %10, %11
  %13 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %12, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %14 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %12, i32 0, i32 8, i32 1, i16 32, i32 undef)
  %15 = mul <8 x i32> %13, %14
  %16 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %15, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %17 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %15, i32 0, i32 4, i32 1, i16 16, i32 undef)
  %18 = mul <4 x i32> %16, %17
  %19 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %18, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %20 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %18, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %21 = mul <2 x i32> %19, %20
  %22 = extractelement <2 x i32> %21, i32 0
  %23 = extractelement <2 x i32> %21, i32 1
  %reduce = mul i32 %22, %23
  ret i32 %reduce
}

attributes #0 = { nofree nosync nounwind readnone willreturn }
