;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck --check-prefix=CHECK-SIMD8 %s
; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S %s | FileCheck --check-prefix=CHECK-SIMD16 %s

declare <128 x float> @llvm.vc.internal.sample.bti.v128f32.v32i1.v32f32(<32 x i1>, i16, i8, i16, i32, i32, <128 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>, <32 x float>)
declare <32 x float> @llvm.vc.internal.sample.bti.v32f32.v8i1.v8f32(<8 x i1>, i16, i8, i16, i32, i32, <32 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <8 x float>)

; CHECK-SIMD8-LABEL: @test_simd32(
; CHECK-SIMD16-LABEL: @test_simd32(
define <128 x float> @test_simd32(<32 x i1> %pred, i32 %bti, i32 %sampler, <32 x float> %u, <32 x float> %v, <32 x float> %r) {
; CHECK-SIMD8: [[PRED0:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK-SIMD8: [[U0:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %u, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[V0:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %v, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[R0:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %r, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[SAMPLE0:%.*]] = call <64 x float> @llvm.vc.internal.sample.bti.v64f32.v16i1.v16f32(<16 x i1> [[PRED0]], i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <64 x float> undef, <16 x float> [[U0]], <16 x float> [[V0]], <16 x float> [[R0]], <16 x float> zeroinitializer
; CHECK-SIMD8: [[INS0:%.*]] = call <128 x float> @llvm.genx.wrregionf.v128f32.v64f32.i16.i1(<128 x float> undef, <64 x float> [[SAMPLE0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD8: [[PRED1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK-SIMD8: [[U1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %u, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[V1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %v, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[R1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %r, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[SAMPLE1:%.*]] = call <64 x float> @llvm.vc.internal.sample.bti.v64f32.v16i1.v16f32(<16 x i1> [[PRED1]], i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <64 x float> undef, <16 x float> [[U1]], <16 x float> [[V1]], <16 x float> [[R1]], <16 x float> zeroinitializer
; CHECK-SIMD8: [[INS1:%.*]] = call <128 x float> @llvm.genx.wrregionf.v128f32.v64f32.i16.i1(<128 x float> [[INS0]], <64 x float> [[SAMPLE1]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK-SIMD8: ret <128 x float> [[INS1]]

; CHECK-SIMD16: %sample = call <128 x float> @llvm.vc.internal.sample.bti.v128f32.v32i1.v32f32(<32 x i1> %pred, i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <128 x float> undef, <32 x float> %u, <32 x float> %v, <32 x float> %r, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer)
  %sample = call <128 x float> @llvm.vc.internal.sample.bti.v128f32.v32i1.v32f32(<32 x i1> %pred, i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <128 x float> undef, <32 x float> %u, <32 x float> %v, <32 x float> %r, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer, <32 x float> zeroinitializer)
  ret <128 x float> %sample
}

; CHECK-SIMD8-LABEL: @test_simd8(
; CHECK-SIMD16-LABEL: @test_simd8(
define <32 x float> @test_simd8(<8 x i1> %pred, i32 %bti, i32 %sampler, <8 x float> %u, <8 x float> %v, <8 x float> %r) {
; CHECK-SIMD8: %sample = call <32 x float> @llvm.vc.internal.sample.bti.v32f32.v8i1.v8f32(<8 x i1> %pred, i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <32 x float> undef, <8 x float> %u, <8 x float> %v, <8 x float> %r, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer)

; CHECK-SIMD16: [[PRED:%.*]] = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> %pred, i32 0)
; CHECK-SIMD16: [[U:%.*]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> undef, <8 x float> %u, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[V:%.*]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> undef, <8 x float> %v, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[R:%.*]] = call <16 x float> @llvm.genx.wrregionf.v16f32.v8f32.i16.i1(<16 x float> undef, <8 x float> %r, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[SAMPLE:%.*]] = call <64 x float> @llvm.vc.internal.sample.bti.v64f32.v16i1.v16f32(<16 x i1> [[PRED]], i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <64 x float> undef, <16 x float> [[U]], <16 x float> [[V]], <16 x float> [[R]], <16 x float> zeroinitializer
; CHECK-SIMD16: [[RES:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> [[SAMPLE]], i32 16, i32 8, i32 1, i16 0, i32 undef)
; CHECK-SIMD16: ret <32 x float> [[RES]]
  %sample = call <32 x float> @llvm.vc.internal.sample.bti.v32f32.v8i1.v8f32(<8 x i1> %pred, i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <32 x float> undef, <8 x float> %u, <8 x float> %v, <8 x float> %r, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer, <8 x float> zeroinitializer)
  ret <32 x float> %sample
}
