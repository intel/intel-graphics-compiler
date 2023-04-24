;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing \
; RUN:     -march=genx64 -mcpu=XeLP -mtriple=spir64 -S < %s | FileCheck %s

declare <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32, i64)
declare <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64>, <1 x i64>, i32, i32, i32, i16, i32, i1)
declare i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64, i64, i32, i32, i32, i16, i32, i1)
declare i64 @llvm.genx.write.predef.reg.i64.i64(i32, i64)
declare <8 x i64> @llvm.genx.wrregioni.v8i64.i64.i16.i1(<8 x i64>, i64, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: test
define <8 x i64> @test() {
; CHECK: [[PREDEFREG:%[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
  %1 = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> [[PREDEFREG]], i32 0, i32 1, i32 1, i16 0, i32 undef)
  %2 = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %1, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[WRR:%[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> [[RDREGION]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %3 = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %2, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[BITCAST:%[^ ]+]] = bitcast <1 x i64> [[WRR]] to i64
  %4 = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %3, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %5 = add i64 %4, 96
  %6 = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %5, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %7 = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %6)
; CHECK: [[WRREGION:%[^ ]+]] = call <8 x i64> @llvm.genx.wrregioni.v8i64.v1i64.i16.i1(<8 x i64> undef, <1 x i64> [[WRR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %8 = call <8 x i64> @llvm.genx.wrregioni.v8i64.i64.i16.i1(<8 x i64> undef, i64 %4, i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
  ret <8 x i64> %8
}
