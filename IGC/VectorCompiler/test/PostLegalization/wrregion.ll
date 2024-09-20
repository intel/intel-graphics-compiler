;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

declare <48 x i32> @llvm.genx.wrregioni.v48i32.v32i32.i16.i1(<48 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)
declare <32 x i16> @llvm.genx.rdregioni.v32i16.v96i16.i16(<96 x i16>, i32, i32, i32, i16, i32)

; CHECK-LABEL: test1
define <32 x i16> @test1(<32 x i32> %arg) {
; CHECK-NEXT: call <40 x i32> @llvm.genx.wrregioni.v40i32.v32i32.i16.i1(<40 x i32> zeroinitializer, <32 x i32> %arg, i32 0, i32 32, i32 1, i16 32, i32 undef, i1 true)
  %1 = call <48 x i32> @llvm.genx.wrregioni.v48i32.v32i32.i16.i1(<48 x i32> zeroinitializer, <32 x i32> %arg, i32 0, i32 32, i32 1, i16 64, i32 undef, i1 true)
  %cast = bitcast <48 x i32> %1 to <192 x i8>
  %postcast = bitcast <192 x i8> %cast to <96 x i16>
  %split = call <32 x i16> @llvm.genx.rdregioni.v32i16.v96i16.i16(<96 x i16> %postcast, i32 32, i32 32, i32 1, i16 62, i32 undef)
  ret <32 x i16> %split
}

declare <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32>, i32, i32, i32, i16, i32)
declare <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32>, i32, i32, i32, i16, i32)
declare <128 x float> @llvm.genx.dpas.nosrc0.v128f32.v128i32.v64i32(<128 x i32>, <64 x i32>, i32)
declare <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float>, i32, i32, i32, i16, i32)
declare <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32)
declare <256 x float> @llvm.genx.wrregionf.v256f32.v128f32.i16.i1(<256 x float>, <128 x float>, i32, i32, i32, i16, i32, i1)
declare <64 x i64> @llvm.genx.rdregioni.v64i64.v128i64.i16(<128 x i64>, i32, i32, i32, i16, i32)

; CHECK-LABEL: test2
define <64 x i64> @test2(<256 x i32> %src1, <128 x i32> %src2) {
  ; CHECK-NEXT: %[[DPAS1_1:[^ ]+]] = tail call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src1, i32 0, i32 128, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: %[[DPAS1_2:[^ ]+]] = tail call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src2, i32 0, i32 64, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: %[[DPAS1_D:[^ ]+]] = call <128 x float> @llvm.genx.dpas.nosrc0.v128f32.v128i32.v64i32(<128 x i32> %[[DPAS1_1]], <64 x i32> %[[DPAS1_2]], i32 134744329)
  ; CHECK-NEXT: %[[DPAS2_1:[^ ]+]] = tail call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src1, i32 0, i32 128, i32 1, i16 512, i32 undef)
  ; CHECK-NEXT: %[[DPAS2_2:[^ ]+]] = tail call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src2, i32 0, i32 64, i32 1, i16 256, i32 undef)
  ; CHECK-NEXT: tail call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> zeroinitializer, <128 x i32> %[[DPAS2_1]], <64 x i32> %[[DPAS2_2]], i32 9, i32 9, i32 8, i32 8, i32 0, i32 0)
  ; CHECK-NEXT: %[[RET:[^ ]+]] = bitcast <128 x float> %[[DPAS1_D]] to <64 x i64>
  ; CHECK-NEXT: ret <64 x i64> %[[RET]]

  %1 = tail call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src1, i32 0, i32 128, i32 1, i16 0, i32 undef)
  %2 = tail call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src2, i32 0, i32 64, i32 1, i16 0, i32 undef)
  %3 = call <128 x float> @llvm.genx.dpas.nosrc0.v128f32.v128i32.v64i32(<128 x i32> %1, <64 x i32> %2, i32 134744329)
  %4 = tail call <256 x float> @llvm.genx.wrregionf.v256f32.v128f32.i16.i1(<256 x float> zeroinitializer, <128 x float> %3, i32 0, i32 128, i32 1, i16 0, i32 undef, i1 true)
  %5 = tail call <128 x float> @llvm.genx.rdregionf.v128f32.v256f32.i16(<256 x float> %4, i32 0, i32 128, i32 1, i16 512, i32 undef)
  %6 = tail call <128 x i32> @llvm.genx.rdregioni.v128i32.v256i32.i16(<256 x i32> %src1, i32 0, i32 128, i32 1, i16 512, i32 undef)
  %7 = tail call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src2, i32 0, i32 64, i32 1, i16 256, i32 undef)
  %8 = tail call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> %5, <128 x i32> %6, <64 x i32> %7, i32 9, i32 9, i32 8, i32 8, i32 0, i32 0)
  %9 = tail call <256 x float> @llvm.genx.wrregionf.v256f32.v128f32.i16.i1(<256 x float> %4, <128 x float> %8, i32 0, i32 128, i32 1, i16 512, i32 undef, i1 true)
  %10 = bitcast <256 x float> %9 to <128 x i64>
  %11 = call <64 x i64> @llvm.genx.rdregioni.v64i64.v128i64.i16(<128 x i64> %10, i32 0, i32 64, i32 1, i16 0, i32 undef)

  ret <64 x i64> %11
}
