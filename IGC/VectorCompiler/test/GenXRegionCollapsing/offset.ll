;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing \
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

