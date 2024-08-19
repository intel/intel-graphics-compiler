;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x float> @llvm.genx.load.v16f32.v16i32(i32, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <64 x float> @llvm.genx.3d.load.v64f32.v16i1.v16i32.v16i32.v16i32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32(i32, <16 x i1>, i32, i16, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>)

declare <16 x float> @llvm.genx.sample.v16f32.v16i32(i32, i32, i32, <16 x float>, <16 x float>, <16 x float>)
declare <64 x float> @llvm.genx.3d.sample.v64f32.v16i1.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32(i32, <16 x i1>, i32, i16, i32, i32, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>)

; CHECK-LABEL: @test_load_simple(
define <16 x float> @test_load_simple(i32 %bti, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r) {
; CHECK: %load = call <16 x float> @llvm.vc.internal.sampler.load.bti.v16f32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 26, i8 1, i16 0, i32 %bti, <16 x float> undef, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer)
  %load = call <16 x float> @llvm.genx.load.v16f32.v16i32(i32 1, i32 %bti, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r)
  ret <16 x float> %load
}

; CHECK-LABEL: @test_load_complex(
define <64 x float> @test_load_complex(<16 x i1> %pred, i32 %bti, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r) {
; CHECK: %load = call <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1> %pred, i16 26, i8 15, i16 0, i32 %bti, <64 x float> undef, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer)
  %load = call <64 x float> @llvm.genx.3d.load.v64f32.v16i1.v16i32.v16i32.v16i32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32(i32 26, <16 x i1> %pred, i32 15, i16 0, i32 %bti, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer)
  ret <64 x float> %load
}

; CHECK-LABEL: @test_sample_simple(
define <16 x float> @test_sample_simple(i32 %bti, i32 %sampler, <16 x float> %u, <16 x float> %v, <16 x float> %r) {
; CHECK: %sample = call <16 x float> @llvm.vc.internal.sample.bti.v16f32.v16i1.v16f32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i32 %bti, i32 %sampler, <16 x float> undef, <16 x float> %u, <16 x float> %v, <16 x float> %r, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer)
  %sample = call <16 x float> @llvm.genx.sample.v16f32.v16i32(i32 1, i32 %sampler, i32 %bti, <16 x float> %u, <16 x float> %v, <16 x float> %r)
  ret <16 x float> %sample
}

; CHECK-LABEL: @test_sample_complex(
define <64 x float> @test_sample_complex(<16 x i1> %pred, i32 %bti, i32 %sampler, <16 x float> %u, <16 x float> %v, <16 x float> %r) {
; CHECK: %sample = call <64 x float> @llvm.vc.internal.sample.bti.v64f32.v16i1.v16f32(<16 x i1> %pred, i16 0, i8 15, i16 0, i32 %bti, i32 %sampler, <64 x float> undef, <16 x float> %u, <16 x float> %v, <16 x float> %r, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer)
  %sample = call <64 x float> @llvm.genx.3d.sample.v64f32.v16i1.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32.v16f32(i32 0, <16 x i1> %pred, i32 15, i16 0, i32 %sampler, i32 %bti, <16 x float> %u, <16 x float> %v, <16 x float> %r, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer)
  ret <64 x float> %sample
}
