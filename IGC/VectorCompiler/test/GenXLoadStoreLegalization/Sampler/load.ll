;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck --check-prefix=CHECK-SIMD8 %s
; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S %s | FileCheck --check-prefix=CHECK-SIMD16 %s

declare <128 x float> @llvm.vc.internal.sampler.load.bti.v128f32.v32i1.v32i32(<32 x i1>, i16, i8, i16, i32, <128 x float>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>)
declare <32 x float> @llvm.vc.internal.sampler.load.bti.v32f32.v8i1.v8i32(<8 x i1>, i16, i8, i16, i32, <32 x float>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>, <8 x i32>)

; CHECK-SIMD8-LABEL: @test_simd32(
; CHECK-SIMD16-LABEL: @test_simd32(
define <128 x float> @test_simd32(<32 x i1> %pred, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> %r) {
; CHECK-SIMD8: [[PRED0:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK-SIMD8: [[U0:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %u, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[V0:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %v, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[R0:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %r, i32 32, i32 16, i32 1, i16 0, i32 undef)
; CHECK-SIMD8: [[LOAD0:%.*]] = call <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1> [[PRED0]], i16 26, i8 15, i16 0, i32 %bti, <64 x float> undef, <16 x i32> [[U0]], <16 x i32> [[V0]], <16 x i32> [[R0]], <16 x i32> zeroinitializer
; CHECK-SIMD8: [[INS0:%.*]] = call <128 x float> @llvm.genx.wrregionf.v128f32.v64f32.i16.i1(<128 x float> undef, <64 x float> [[LOAD0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD8: [[PRED1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK-SIMD8: [[U1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %u, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[V1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %v, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[R1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %r, i32 32, i32 16, i32 1, i16 64, i32 undef)
; CHECK-SIMD8: [[LOAD1:%.*]] = call <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1> [[PRED1]], i16 26, i8 15, i16 0, i32 %bti, <64 x float> undef, <16 x i32> [[U1]], <16 x i32> [[V1]], <16 x i32> [[R1]], <16 x i32> zeroinitializer
; CHECK-SIMD8: [[INS1:%.*]] = call <128 x float> @llvm.genx.wrregionf.v128f32.v64f32.i16.i1(<128 x float> [[INS0]], <64 x float> [[LOAD1]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK-SIMD8: ret <128 x float> [[INS1]]

; CHECK-SIMD16: %load = call <128 x float> @llvm.vc.internal.sampler.load.bti.v128f32.v32i1.v32i32(<32 x i1> %pred, i16 26, i8 15, i16 0, i32 %bti, <128 x float> undef, <32 x i32> %u, <32 x i32> %v, <32 x i32> %r, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer)
  %load = call <128 x float> @llvm.vc.internal.sampler.load.bti.v128f32.v32i1.v32i32(<32 x i1> %pred, i16 26, i8 15, i16 0, i32 %bti, <128 x float> undef, <32 x i32> %u, <32 x i32> %v, <32 x i32> %r, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer)
  ret <128 x float> %load
}

; CHECK-SIMD8-LABEL: @test_simd8(
; CHECK-SIMD16-LABEL: @test_simd8(
define <32 x float> @test_simd8(<8 x i1> %pred, i32 %bti, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r) {
; CHECK-SIMD8: %load = call <32 x float> @llvm.vc.internal.sampler.load.bti.v32f32.v8i1.v8i32(<8 x i1> %pred, i16 26, i8 15, i16 0, i32 %bti, <32 x float> undef, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer)

; CHECK-SIMD16: [[PRED:%.*]] = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> %pred, i32 0)
; CHECK-SIMD16: [[U:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %u, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[V:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %v, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[R:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %r, i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-SIMD16: [[SAMPLE:%.*]] = call <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1> [[PRED]], i16 26, i8 15, i16 0, i32 %bti, <64 x float> undef, <16 x i32> [[U]], <16 x i32> [[V]], <16 x i32> [[R]], <16 x i32> zeroinitializer
; CHECK-SIMD16: [[RES:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v64f32.i16(<64 x float> [[SAMPLE]], i32 16, i32 8, i32 1, i16 0, i32 undef)
; CHECK-SIMD16: ret <32 x float> [[RES]]
  %load = call <32 x float> @llvm.vc.internal.sampler.load.bti.v32f32.v8i1.v8i32(<8 x i1> %pred, i16 26, i8 15, i16 0, i32 %bti, <32 x float> undef, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer)
  ret <32 x float> %load
}
