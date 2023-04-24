;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32)
declare <32 x double> @llvm.genx.wrregionf.v32f64.v30f64.i16.i1(<32 x double>, <30 x double>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_split1
; CHECK:      [[var1:[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %data, i32 2, i32 4, i32 1, i16 %offset, i32 0)
; CHECK-NEXT: [[var2:[^ ]+]] = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> undef, <4 x i32> [[var1]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[var3:[^ ]+]] = add i16 %offset, 8
; CHECK-NEXT: [[var4:[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %data, i32 2, i32 4, i32 1, i16 [[var3]], i32 0)
; CHECK-NEXT: [[var5:[^ ]+]] = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> [[var2]], <4 x i32> [[var4]], i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT: ret <8 x i32> [[var5]]
define <8 x i32> @test_split1(<16 x i32> %data, i16 %offset) {
  %result = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 2, i32 4, i32 1, i16 %offset, i32 undef)
  ret <8 x i32> %result
}

; CHECK-LABEL: @test_split2
; CHECK:      [[var1:[^ ]+]]  = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %data, i32 4, i32 4, i32 1, i16 %offset, i32 0)
; CHECK-NEXT: [[var2:[^ ]+]]  = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> undef, <4 x i32> [[var1]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[var3:[^ ]+]]  = add i16 %offset, 16
; CHECK-NEXT: [[var4:[^ ]+]]  = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> %data, i32 1, i32 1, i32 1, i16 [[var3]], i32 0)
; CHECK-NEXT: [[var5:[^ ]+]]  = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> [[var2]], <1 x i32> [[var4]], i32 0, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT: [[var6:[^ ]+]]  = add i16 %offset, 20
; CHECK-NEXT: [[var7:[^ ]+]]  = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> %data, i32 1, i32 1, i32 1, i16 [[var6]], i32 0)
; CHECK-NEXT: [[var8:[^ ]+]]  = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> [[var5]], <1 x i32> [[var7]], i32 0, i32 1, i32 1, i16 20, i32 undef, i1 true)
; CHECK-NEXT: [[var9:[^ ]+]]  = add i16 %offset, 24
; CHECK-NEXT: [[var10:[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> %data, i32 1, i32 1, i32 1, i16 [[var9]], i32 0)
; CHECK-NEXT: [[var11:[^ ]+]] = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> [[var8]], <1 x i32> [[var10]], i32 0, i32 1, i32 1, i16 24, i32 undef, i1 true)
; CHECK-NEXT: [[var12:[^ ]+]] = add i16 %offset, 8
; CHECK-NEXT: [[var13:[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> %data, i32 1, i32 1, i32 1, i16 [[var12]], i32 0)
; CHECK-NEXT: [[var14:[^ ]+]] = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> [[var11]], <1 x i32> [[var13]], i32 0, i32 1, i32 1, i16 28, i32 undef, i1 true)
; CHECK-NEXT: ret <8 x i32> [[var14]]
define <8 x i32> @test_split2(<16 x i32> %data, i16 %offset) {
  %result = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 2, i32 7, i32 1, i16 %offset, i32 undef)
  ret <8 x i32> %result
}

; CHECK-LABEL: @test_no_split1
; CHECK:      [[var1:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 0, i32 8, i32 1, i16 %offset, i32 0)
; CHECK-NEXT: ret <8 x i32> [[var1]]
define <8 x i32> @test_no_split1(<16 x i32> %data, i16 %offset) {
  %result = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 1, i32 1, i32 0, i16 %offset, i32 undef)
  ret <8 x i32> %result
}

; CHECK-LABEL: @test_no_split2
; CHECK:      [[var1:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 0, i32 8, i32 1, i16 %offset, i32 0)
; CHECK-NEXT: ret <8 x i32> [[var1]]
define <8 x i32> @test_no_split2(<16 x i32> %data, i16 %offset) {
  %result = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %data, i32 0, i32 8, i32 1, i16 %offset, i32 undef)
  ret <8 x i32> %result
}

; CHECK-LABEL: @test_single_wrregion
; CHECK-NEXT: [[var1:[^ ]+]]  = call <8 x double> @llvm.genx.rdregionf.v8f64.v30f64.i16(<30 x double> %data, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[var2:[^ ]+]]  = call <32 x double> @llvm.genx.wrregionf.v32f64.v8f64.i16.i1(<32 x double> undef, <8 x double> [[var1]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[var3:[^ ]+]]  = call <4 x double> @llvm.genx.rdregionf.v4f64.v30f64.i16(<30 x double> %data, i32 4, i32 4, i32 1, i16 64, i32 undef)
; CHECK-NEXT: [[var4:[^ ]+]]  = call <32 x double> @llvm.genx.wrregionf.v32f64.v4f64.i16.i1(<32 x double> [[var2]], <4 x double> [[var3]], i32 4, i32 4, i32 1, i16 64, i32 undef, i1 true)
; CHECK-NEXT: [[var5:[^ ]+]]  = call <2 x double> @llvm.genx.rdregionf.v2f64.v30f64.i16(<30 x double> %data, i32 2, i32 2, i32 1, i16 96, i32 undef)
; CHECK-NEXT: [[var6:[^ ]+]]  = call <32 x double> @llvm.genx.wrregionf.v32f64.v2f64.i16.i1(<32 x double> [[var4]], <2 x double> [[var5]], i32 2, i32 2, i32 1, i16 96, i32 undef, i1 true)
; CHECK-NEXT: [[var7:[^ ]+]]  = call <1 x double> @llvm.genx.rdregionf.v1f64.v30f64.i16(<30 x double> %data, i32 1, i32 1, i32 1, i16 112, i32 undef)
; CHECK-NEXT: [[var8:[^ ]+]]  = call <32 x double> @llvm.genx.wrregionf.v32f64.v1f64.i16.i1(<32 x double> [[var6]], <1 x double> [[var7]], i32 1, i32 1, i32 1, i16 112, i32 undef, i1 true)
; CHECK-NEXT: [[var9:[^ ]+]]  = call <4 x double> @llvm.genx.rdregionf.v4f64.v30f64.i16(<30 x double> %data, i32 4, i32 4, i32 1, i16 120, i32 undef)
; CHECK-NEXT: [[var10:[^ ]+]] = call <32 x double> @llvm.genx.wrregionf.v32f64.v4f64.i16.i1(<32 x double> [[var8]], <4 x double> [[var9]], i32 4, i32 4, i32 1, i16 128, i32 undef, i1 true)
; CHECK-NEXT: [[var11:[^ ]+]] = call <4 x double> @llvm.genx.rdregionf.v4f64.v30f64.i16(<30 x double> %data, i32 4, i32 4, i32 1, i16 152, i32 undef)
; CHECK-NEXT: [[var12:[^ ]+]] = call <32 x double> @llvm.genx.wrregionf.v32f64.v4f64.i16.i1(<32 x double> [[var10]], <4 x double> [[var11]], i32 4, i32 4, i32 1, i16 160, i32 undef, i1 true)
; CHECK-NEXT: [[var13:[^ ]+]] = call <4 x double> @llvm.genx.rdregionf.v4f64.v30f64.i16(<30 x double> %data, i32 4, i32 4, i32 1, i16 184, i32 undef)
; CHECK-NEXT: [[var14:[^ ]+]] = call <32 x double> @llvm.genx.wrregionf.v32f64.v4f64.i16.i1(<32 x double> [[var12]], <4 x double> [[var13]], i32 4, i32 4, i32 1, i16 192, i32 undef, i1 true)
; CHECK-NEXT: [[var15:[^ ]+]] = call <2 x double> @llvm.genx.rdregionf.v2f64.v30f64.i16(<30 x double> %data, i32 2, i32 2, i32 1, i16 216, i32 undef)
; CHECK-NEXT: [[var16:[^ ]+]] = call <32 x double> @llvm.genx.wrregionf.v32f64.v2f64.i16.i1(<32 x double> [[var14]], <2 x double> [[var15]], i32 2, i32 2, i32 1, i16 224, i32 undef, i1 true)
; CHECK-NEXT: [[var17:[^ ]+]] = call <1 x double> @llvm.genx.rdregionf.v1f64.v30f64.i16(<30 x double> %data, i32 1, i32 1, i32 1, i16 232, i32 undef)
; CHECK-NEXT: [[var18:[^ ]+]] = call <32 x double> @llvm.genx.wrregionf.v32f64.v1f64.i16.i1(<32 x double> [[var16]], <1 x double> [[var17]], i32 1, i32 1, i32 1, i16 240, i32 undef, i1 true)
; CHECK-NEXT: ret <32 x double> [[var18]]
define <32 x double> @test_single_wrregion(<30 x double> %data) {
  %result = call <32 x double> @llvm.genx.wrregionf.v32f64.v30f64.i16.i1(<32 x double> undef, <30 x double> %data, i32 16, i32 15, i32 1, i16 0, i32 undef, i1 true)
  ret <32 x double> %result
}

