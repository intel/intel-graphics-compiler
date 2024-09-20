;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double>, i32, i32, i32, i16, i32)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <2 x i64>)
declare <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double>, <4 x double>, i32, i32, i32, i16, i32, i1)
declare <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double>, i32, i32, i32, i16, i32)
declare <24 x double> @llvm.genx.wrregionf.v24f64.v2f64.i16.i1(<24 x double>, <2 x double>, i32, i32, i32, i16, i32, i1)
declare <4 x double> @llvm.genx.rdregionf.v4f64.v24f64.i16(<24 x double>, i32, i32, i32, i16, i32)
declare <2 x double> @llvm.genx.rdregionf.v2f64.v24f64.i16(<24 x double>, i32, i32, i32, i16, i32)

define void @foo(<64 x i64> %src, i64 %ptrtoint) {
; CHECK: %[[BITCAST1:[^ ]+]] = bitcast <64 x i64> %src to <64 x double>
; CHECK: %[[SPLIT1:[^ ]+]] = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %[[BITCAST1]], i32 4, i32 4, i32 1, i16 128, i32 undef)
; CHECK: %[[DECOMP1:[^ ]+]] = call <8 x double> @llvm.genx.wrregionf.v8f64.v4f64.i16.i1(<8 x double> undef, <4 x double> %[[SPLIT1]], i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
; CHECK: %[[SPLIT2:[^ ]+]] = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %[[BITCAST1]], i32 2, i32 2, i32 1, i16 160, i32 undef)
; CHECK: %[[DECOMP2:[^ ]+]] = call <8 x double> @llvm.genx.wrregionf.v8f64.v2f64.i16.i1(<8 x double> %[[DECOMP1]], <2 x double> %[[SPLIT2]], i32 0, i32 2, i32 1, i16 48, i32 undef, i1 true)
; CHECK: %[[SPLIT3:[^ ]+]] = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %[[BITCAST1]], i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK: %[[FDIV1:[^ ]+]] = fdiv <4 x double> <double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00>, %[[SPLIT3]]
; CHECK: %[[SPLIT4:[^ ]+]] = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %[[BITCAST1]], i32 2, i32 2, i32 1, i16 32, i32 undef)
; CHECK: %[[FDIV2:[^ ]+]] = fdiv <2 x double> <double 1.000000e+00, double 1.000000e+00>, %[[SPLIT4]]
; CHECK: %[[DECOMP3:[^ ]+]] = call <8 x double> @llvm.genx.wrregionf.v8f64.v2f64.i16.i1(<8 x double> %[[DECOMP2]], <2 x double> %[[FDIV2]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[SPLIT5:[^ ]+]] = call <4 x double> @llvm.genx.rdregionf.v4f64.v8f64.i16(<8 x double> %[[DECOMP3]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; CHECK: %[[SPLIT6:[^ ]+]] = fmul <4 x double> %[[FDIV1]], %[[SPLIT5]]
; CHECK: %[[DECOMP4:[^ ]+]] = call <8 x double> @llvm.genx.wrregionf.v8f64.v4f64.i16.i1(<8 x double> %[[DECOMP3]], <4 x double> %[[SPLIT6]], i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
; CHECK: %[[SPLIT7:[^ ]+]] = call <2 x double> @llvm.genx.rdregionf.v2f64.v8f64.i16(<8 x double> %[[DECOMP4]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; CHECK: %[[SPLIT8:[^ ]+]] = call <2 x double> @llvm.genx.rdregionf.v2f64.v8f64.i16(<8 x double> %[[DECOMP4]], i32 0, i32 2, i32 1, i16 48, i32 undef)
; CHECK: fmul <2 x double> %[[SPLIT7]], %[[SPLIT8]]

  %.bitcast_before_collapse = bitcast <64 x i64> %src to <64 x double>
  %.esimd3.split6 = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 4, i32 4, i32 1, i16 128, i32 undef)
  %.esimd3.split6.join6 = call <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double> undef, <4 x double> %.esimd3.split6, i32 0, i32 4, i32 1, i16 48, i32 undef, i1 true)
  %.esimd3.split10 = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 2, i32 2, i32 1, i16 160, i32 undef)
  %.esimd3.split10.join10 = call <24 x double> @llvm.genx.wrregionf.v24f64.v2f64.i16.i1(<24 x double> %.esimd3.split6.join6, <2 x double> %.esimd3.split10, i32 0, i32 2, i32 1, i16 80, i32 undef, i1 true)
  %.esimd3.split12 = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 4, i32 4, i32 1, i16 256, i32 undef)
  %.esimd3.split12.join12 = call <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double> %.esimd3.split10.join10, <4 x double> %.esimd3.split12, i32 0, i32 4, i32 1, i16 96, i32 undef, i1 true)
  %.esimd3.split16 = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 2, i32 2, i32 1, i16 288, i32 undef)
  %.esimd3.split16.join16 = call <24 x double> @llvm.genx.wrregionf.v24f64.v2f64.i16.i1(<24 x double> %.esimd3.split12.join12, <2 x double> %.esimd3.split16, i32 0, i32 2, i32 1, i16 128, i32 undef, i1 true)
  %.esimd3.split18 = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 4, i32 4, i32 1, i16 384, i32 undef)
  %.esimd3.split18.join18 = call <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double> %.esimd3.split16.join16, <4 x double> %.esimd3.split18, i32 0, i32 4, i32 1, i16 144, i32 undef, i1 true)
  %.esimd3.split22 = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 2, i32 2, i32 1, i16 416, i32 undef)
  %.esimd3.split22.join22 = call <24 x double> @llvm.genx.wrregionf.v24f64.v2f64.i16.i1(<24 x double> %.esimd3.split18.join18, <2 x double> %.esimd3.split22, i32 0, i32 2, i32 1, i16 176, i32 undef, i1 true)
  %.esimd9.regioncollapsed.split0 = call <4 x double> @llvm.genx.rdregionf.v4f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 4, i32 4, i32 1, i16 0, i32 undef)
  %.split0258 = fdiv <4 x double> <double 1.000000e+00, double 1.000000e+00, double 1.000000e+00, double 1.000000e+00>, %.esimd9.regioncollapsed.split0
  %.esimd10.join0 = call <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double> %.esimd3.split22.join22, <4 x double> %.split0258, i32 4, i32 4, i32 1, i16 0, i32 undef, i1 true)
  %.esimd9.regioncollapsed.split4 = call <2 x double> @llvm.genx.rdregionf.v2f64.v64f64.i16(<64 x double> %.bitcast_before_collapse, i32 2, i32 2, i32 1, i16 32, i32 undef)
  %.split4259 = fdiv <2 x double> <double 1.000000e+00, double 1.000000e+00>, %.esimd9.regioncollapsed.split4
  %.esimd10.join4 = call <24 x double> @llvm.genx.wrregionf.v24f64.v2f64.i16.i1(<24 x double> %.esimd10.join0, <2 x double> %.split4259, i32 2, i32 2, i32 1, i16 32, i32 undef, i1 true)
  %.esimd11.split0 = call <4 x double> @llvm.genx.rdregionf.v4f64.v24f64.i16(<24 x double> %.esimd10.join4, i32 4, i32 4, i32 1, i16 0, i32 undef)
  %.esimd12.split0 = call <4 x double> @llvm.genx.rdregionf.v4f64.v24f64.i16(<24 x double> %.esimd10.join4, i32 4, i32 4, i32 1, i16 48, i32 undef)
  %.split0256 = fmul <4 x double> %.esimd11.split0, %.esimd12.split0
  %.esimd13.join0 = call <24 x double> @llvm.genx.wrregionf.v24f64.v4f64.i16.i1(<24 x double> %.esimd10.join4, <4 x double> %.split0256, i32 4, i32 4, i32 1, i16 48, i32 undef, i1 true)
  %.esimd11.split4 = call <2 x double> @llvm.genx.rdregionf.v2f64.v24f64.i16(<24 x double> %.esimd13.join0, i32 2, i32 2, i32 1, i16 32, i32 undef)
  %.esimd12.split4 = call <2 x double> @llvm.genx.rdregionf.v2f64.v24f64.i16(<24 x double> %.esimd13.join0, i32 2, i32 2, i32 1, i16 80, i32 undef)
  %.split4257 = fmul <2 x double> %.esimd11.split4, %.esimd12.split4
  %bitcast3 = bitcast <2 x double> %.split4257 to <2 x i64>
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %ptrtoint, i16 1, i32 0, <2 x i64> %bitcast3)
  ret void
}
