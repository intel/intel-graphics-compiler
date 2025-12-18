;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXRegionCollapsing \
; RUN:     -march=genx64 -mcpu=Xe3P -mtriple=spir64 -S < %s | FileCheck %s

; RUN: %opt_new_pm_typed -passes=GenXRegionCollapsing \
; RUN:     -march=genx64 -mcpu=Xe3P -mtriple=spir64 -S < %s | FileCheck %s

declare <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32>, <16 x i16>)

declare <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16>, i32, i32, i32, i16, i32)
declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32)

; CHECK-LABEL: @test_collapse
define <16 x i32> @test_collapse(<16 x i32> %lut, <64 x i16> %src) {
; CHECK: [[COLLAPSED:%.+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v64i16.i16(<64 x i16> %src, i32 0, i32 16, i32 2, i16 66, i32 undef)
; CHECK: %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> [[COLLAPSED]])
  %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 1, i32 1, i32 0, i16 64, i32 0)
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}

; CHECK-LABEL: @test_bitcast
define <16 x i32> @test_bitcast(<16 x i32> %lut, <32 x i32> %src) {
; CHECK: [[CAST:%.+]] = bitcast <32 x i32> %src to <64 x i16>
; CHECK: [[COLLAPSED:%.+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v64i16.i16(<64 x i16> [[CAST]], i32 0, i32 16, i32 2, i16 66, i32 undef)
; CHECK: %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> [[COLLAPSED]])
  %row = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %src, i32 1, i32 1, i32 0, i16 64, i32 0)
  %cast = bitcast <16 x i32> %row to <32 x i16>
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %cast, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}

; CHECK-LABEL: @test_unaligned
define <16 x i32> @test_unaligned(<16 x i32> %lut, <64 x i16> %src) {
; CHECK: %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 1, i32 1, i32 0, i16 60, i32 0)
; CHECK: %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
; CHECK: %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 1, i32 1, i32 0, i16 60, i32 0)
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}

; CHECK-LABEL: @test_strided
define <16 x i32> @test_strided(<16 x i32> %lut, <64 x i16> %src) {
; CHECK: %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 2, i32 1, i32 0, i16 0, i32 0)
; CHECK: %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
; CHECK: %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 2, i32 1, i32 0, i16 0, i32 0)
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}

; CHECK-LABEL: @test_indirect
define <16 x i32> @test_indirect(<16 x i32> %lut, <64 x i16> %src, i16 %offset) {
; CHECK: %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 1, i32 1, i32 0, i16 %offset, i32 0)
; CHECK: %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
; CHECK: %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  %row = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %src, i32 1, i32 1, i32 0, i16 %offset, i32 0)
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %row, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}

; CHECK-LABEL: @test_extend
define <16 x i32> @test_extend(<16 x i32> %lut, <64 x i8> %src) {
; CHECK: %row = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %src, i32 1, i32 1, i32 0, i16 0, i32 0)
; CHECK: %ext = sext <32 x i8> %row to <32 x i16>
; CHECK: %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %ext, i32 2, i32 1, i32 0, i16 2, i32 0)
  %row = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %src, i32 1, i32 1, i32 0, i16 0, i32 0)
  %ext = sext <32 x i8> %row to <32 x i16>
  %odd = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %ext, i32 2, i32 1, i32 0, i16 2, i32 0)
  %upconvert = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %odd)
  ret <16 x i32> %upconvert
}
