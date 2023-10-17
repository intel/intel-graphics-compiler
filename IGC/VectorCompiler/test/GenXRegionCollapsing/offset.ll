;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXRegionCollapsing \
; RUN:     -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <1 x i32> @llvm.genx.lsc.load.stateless.v1i32.i1.i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i64, i32)
declare void @llvm.genx.lsc.store.stateless.i1.i64.v2i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i64, <2 x i32>, i32)
declare i32 @llvm.genx.rdregioni.i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
declare <4 x i16> @llvm.genx.wrregioni.v4i16.v2i16.i16.i1(<4 x i16>, <2 x i16>, i32, i32, i32, i16, i32, i1)

; CHECK:define void @test(i64 [[IN:%[^ ]+]], i64 [[OUT:%[^ ]+]])
; CHECK-NEXT: [[LOAD:%[^ ]+]] = tail call <1 x i32> @llvm.genx.lsc.load.stateless.v1i32.i1.i64(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, i64 [[IN]], i32 0)
; CHECK-NEXT: [[BITCAST1:%[^ ]+]] = bitcast <1 x i32> [[LOAD]] to i32
; CHECK-NEXT: [[BITCAST2:%[^ ]+]] = bitcast i32 [[BITCAST1]] to <2 x i16>
; CHECK-NEXT: [[WRREGION:%[^ ]+]] = tail call <4 x i16> @llvm.genx.wrregioni.v4i16.v2i16.i16.i1(<4 x i16> zeroinitializer, <2 x i16> [[BITCAST2]], i32 0, i32 2, i32 1, i16 2, i32 undef, i1 true)
; CHECK-NEXT: [[BITCAST3:%[^ ]+]] = bitcast <4 x i16> [[WRREGION]] to <2 x i32>
; CHECK-NEXT: tail call void @llvm.genx.lsc.store.stateless.i1.i64.v2i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i64 [[OUT]], <2 x i32> [[BITCAST3]], i32 0)

define void @test(i64 %in, i64 %out) {
  %load = tail call <1 x i32> @llvm.genx.lsc.load.stateless.v1i32.i1.i64(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, i64 %in, i32 0)
  %rdregion= call i32 @llvm.genx.rdregioni.i32.v1i32.i16(<1 x i32> %load, i32 0, i32 1, i32 1, i16 0, i32 0)
  %bitcast1 = bitcast i32 %rdregion to <2 x i16>
  %wrregion = tail call <4 x i16> @llvm.genx.wrregioni.v4i16.v2i16.i16.i1(<4 x i16> zeroinitializer, <2 x i16> %bitcast1, i32 0, i32 2, i32 1, i16 2, i32 undef, i1 true)
  %bitcast2 = bitcast <4 x i16> %wrregion to <2 x i32>
  tail call void @llvm.genx.lsc.store.stateless.i1.i64.v2i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i64 %out, <2 x i32> %bitcast2, i32 0)
  ret void
}

declare <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16(<264 x float>, i32, i32, i32, <8 x i16>, i32)
declare <264 x float> @llvm.genx.wrregionf.v264f32.f32.i16.i1(<264 x float>, float, i32, i32, i32, i16, i32, i1)

; Check that rdregions with non periodic offsets not generated
; CHECK:define void @test2
define void @test2(float %in, <8 x float>* %out) {
  %1 = call <264 x float> @llvm.genx.wrregionf.v264f32.f32.i16.i1(<264 x float> undef, float %in, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; Not fit
  ; CHECK: call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16{{.*}} <i16 24, i16 28, i16 96, i16 100, i16 104, i16 108, i16 112, i16 116>
  %2 = call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16(<264 x float> %1, i32 0, i32 1, i32 0, <8 x i16> <i16 24, i16 28, i16 96, i16 100, i16 104, i16 108, i16 112, i16 116>, i32 0)
  store <8 x float> %2, <8 x float>* %out, align 4
  ; fit
  ; CHECK: call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.i16(<264 x float> %1, i32 20, i32 4, i32 1, i16 24
  %3 = call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16(<264 x float> %1, i32 0, i32 1, i32 0, <8 x i16> <i16 24, i16 28, i16 32, i16 36, i16 104, i16 108, i16 112, i16 116>, i32 0)
  store <8 x float> %3, <8 x float>* %out, align 4
  ; Not fit
  ; CHECK:  call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16{{.*}} <i16 24, i16 28, i16 32, i16 36, i16 40, i16 108, i16 112,
  %4 = call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16(<264 x float> %1, i32 0, i32 1, i32 0, <8 x i16> <i16 24, i16 28, i16 32, i16 36, i16 40, i16 108, i16 112, i16 116>, i32 0)
  store <8 x float> %4, <8 x float>* %out, align 4
  ; Not fit
  ; CHECK:  call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16{{.*}} <i16 24, i16 28, i16 32, i16 36, i16 40, i16 44, i16 112,
  %5 = call <8 x float> @llvm.genx.rdregionf.v8f32.v264f32.v8i16(<264 x float> %1, i32 0, i32 1, i32 0, <8 x i16> <i16 24, i16 28, i16 32, i16 36, i16 40, i16 44, i16 112, i16 116>, i32 0)
  store <8 x float> %5, <8 x float>* %out, align 4
  ret void
}
