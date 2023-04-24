;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: these are tests for bales that shall be done in critical W/Ls
; COM: extracted from fp tests in GCA W/L

declare <16 x double> @llvm.fma.v16f64(<16 x double> %rgn, <16 x double>, <16 x double>);
declare <16 x double> @llvm.genx.rdregionf.v16f64.v64f64.i16(<64 x double>, i32, i32, i32, i16, i32);

define <16 x double> @fmul_corr(<64 x double> %rgn, <16 x double> %val, <16 x double> %val2) {
; CHECK: %1 = call <16 x double> @llvm.genx.rdregionf.v16f64.v64f64.i16(<64 x double> %rgn, i32 0, i32 1, i32 0, i16 8, i32 undef): rdregion
; CHECK: %2 = fneg <16 x double> %val: negmod
; CHECK: %3 = call <16 x double> @llvm.fma.v16f64(<16 x double> %1, <16 x double> %2, <16 x double> %val2): maininst 0 1
  %1 = call <16 x double> @llvm.genx.rdregionf.v16f64.v64f64.i16(<64 x double> %rgn, i32 0, i32 1, i32 0, i16 8, i32 undef)
  %2 = fneg <16 x double> %val
  %3 = call <16 x double> @llvm.fma.v16f64(<16 x double> %1, <16 x double> %2, <16 x double> %val2)
  ret <16 x double> %3
}

declare <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16>, i32, i32, i32, i16, i32)
declare <16 x i64> @llvm.genx.wrregioni.v16i64.v8i64.i16.i1(<16 x i64>, <8 x i64>, i32, i32, i32, i16, i32, i1)

define <16 x i64> @seze_corr(<16 x i16> %val, <16 x i64> %rgn) {
; CHECK: %1 = call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %val, i32 8, i32 8, i32 1, i16 16, i32 undef): rdregion
; CHECK: %2 = zext <8 x i16> %1 to <8 x i32>: zext 0
; CHECK: %3 = sub <8 x i32> zeroinitializer, %2: negmod 1
; CHECK: %4 = sext <8 x i32> %3 to <8 x i64>: sext
; CHECK: %5 = call <16 x i64> @llvm.genx.wrregioni.v16i64.v8i64.i16.i1(<16 x i64> %rgn, <8 x i64> %4, i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true): wrregion 1
  %1 = call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %val, i32 8, i32 8, i32 1, i16 16, i32 undef)
  %2 = zext <8 x i16> %1 to <8 x i32>
  %3 = sub <8 x i32> zeroinitializer, %2
  %4 = sext <8 x i32> %3 to <8 x i64>
  %5 = call <16 x i64> @llvm.genx.wrregioni.v16i64.v8i64.i16.i1(<16 x i64> %rgn, <8 x i64> %4, i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
  ret <16 x i64> %5
}

define <16 x float> @sesi_corr(<16 x i8> %val) {
; CHECK: %1 = sext <16 x i8> %val to <16 x i32>: sext
; CHECK: %2 = sub nsw <16 x i32> zeroinitializer, %1: negmod 1
; CHECK: %3 = sitofp <16 x i32> %2 to <16 x float>: maininst 0
  %1 = sext <16 x i8> %val to <16 x i32>
  %2 = sub nsw <16 x i32> zeroinitializer, %1
  %3 = sitofp <16 x i32> %2 to <16 x float>
  ret <16 x float> %3
}

define <1 x i32> @subse_corr(<1 x i32> %val, <1 x i1> %val2, <1 x i32> %val3) {
; CHECK: %1 = sub <1 x i32> zeroinitializer, %val: negmod
; CHECK: %2 = select <1 x i1> %val2, <1 x i32> %1, <1 x i32> %val3: maininst 1
  %1 = sub <1 x i32> zeroinitializer, %val
  %2 = select <1 x i1> %val2, <1 x i32> %1, <1 x i32> %val3
  ret <1 x i32> %2
}

