;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe2 \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; LLVM 20+ sinks a splat below sqrt, so a broadcast rsqrt arrives as a scalar
; sqrt broadcast into a wide reciprocal. Fold the sqrt and the reciprocal back
; into a scalar rsqrt feeding the same broadcast:
;   (fdiv 1., (splat (sqrt x))) -> (splat (rsqrt x))
;   (inv       (splat (sqrt x))) -> (splat (rsqrt x))

; CHECK-LABEL: @test_fdiv_splat
define <16 x float> @test_fdiv_splat(<1 x float> %x) {
; CHECK: [[RS:%.*]] = call <1 x float> @llvm.genx.rsqrt.v1f32(<1 x float> %x)
; CHECK: [[B:%.*]] = shufflevector <1 x float> [[RS]], <1 x float> poison, <16 x i32> zeroinitializer
; CHECK: ret <16 x float> [[B]]
; CHECK-NOT: @llvm.sqrt
; CHECK-NOT: @llvm.genx.inv
  %s = call afn <1 x float> @llvm.sqrt.v1f32(<1 x float> %x)
  %b = shufflevector <1 x float> %s, <1 x float> poison, <16 x i32> zeroinitializer
  %r = fdiv arcp <16 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %b
  ret <16 x float> %r
}

; CHECK-LABEL: @test_inv_splat
define <16 x float> @test_inv_splat(<1 x float> %x) {
; CHECK: [[RS:%.*]] = call <1 x float> @llvm.genx.rsqrt.v1f32(<1 x float> %x)
; CHECK: [[B:%.*]] = shufflevector <1 x float> [[RS]], <1 x float> poison, <16 x i32> zeroinitializer
; CHECK: ret <16 x float> [[B]]
; CHECK-NOT: @llvm.sqrt
; CHECK-NOT: @llvm.genx.inv
  %s = call afn <1 x float> @llvm.sqrt.v1f32(<1 x float> %x)
  %b = shufflevector <1 x float> %s, <1 x float> poison, <16 x i32> zeroinitializer
  %r = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %b)
  ret <16 x float> %r
}

; After GenXLowering the broadcast is a genx.rdregion, not a shufflevector.
; CHECK-LABEL: @test_rdregion_fdiv
define <16 x float> @test_rdregion_fdiv(<1 x float> %x) {
; CHECK: [[RS:%.*]] = call <1 x float> @llvm.genx.rsqrt.v1f32(<1 x float> %x)
; CHECK: [[B:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float> [[RS]]
; CHECK: ret <16 x float> [[B]]
; CHECK-NOT: call{{.*}}@llvm.genx.sqrt
; CHECK-NOT: fdiv
  %s = call <1 x float> @llvm.genx.sqrt.v1f32(<1 x float> %x)
  %b = call <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float> %s, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %r = fdiv arcp <16 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %b
  ret <16 x float> %r
}

; CHECK-LABEL: @test_rdregion_inv
define <16 x float> @test_rdregion_inv(<1 x float> %x) {
; CHECK: [[RS:%.*]] = call <1 x float> @llvm.genx.rsqrt.v1f32(<1 x float> %x)
; CHECK: [[B:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float> [[RS]]
; CHECK: ret <16 x float> [[B]]
; CHECK-NOT: call{{.*}}@llvm.genx.sqrt
; CHECK-NOT: genx.inv
  %s = call <1 x float> @llvm.genx.sqrt.v1f32(<1 x float> %x)
  %b = call <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float> %s, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %r = call <16 x float> @llvm.genx.inv.v16f32(<16 x float> %b)
  ret <16 x float> %r
}

; A non-splat shuffle (mask is not all-zero) must not be folded.
; CHECK-LABEL: @test_not_splat
define <4 x float> @test_not_splat(<2 x float> %x) {
; CHECK: call afn <2 x float> @llvm.sqrt.v2f32
; CHECK-NOT: call{{.*}}rsqrt
  %s = call afn <2 x float> @llvm.sqrt.v2f32(<2 x float> %x)
  %b = shufflevector <2 x float> %s, <2 x float> poison, <4 x i32> <i32 0, i32 1, i32 0, i32 1>
  %r = call <4 x float> @llvm.genx.inv.v4f32(<4 x float> %b)
  ret <4 x float> %r
}

; A plain (non-approx) llvm.sqrt with a plain fdiv must not become rsqrt.
; CHECK-LABEL: @test_not_fast
define <16 x float> @test_not_fast(<1 x float> %x) {
; CHECK: call <1 x float> @llvm.sqrt.v1f32
; CHECK-NOT: call{{.*}}rsqrt
  %s = call <1 x float> @llvm.sqrt.v1f32(<1 x float> %x)
  %b = shufflevector <1 x float> %s, <1 x float> poison, <16 x i32> zeroinitializer
  %r = fdiv <16 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %b
  ret <16 x float> %r
}

declare <1 x float> @llvm.sqrt.v1f32(<1 x float>)
declare <2 x float> @llvm.sqrt.v2f32(<2 x float>)
declare <1 x float> @llvm.genx.sqrt.v1f32(<1 x float>)
declare <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float>, i32, i32, i32, i16, i32)
declare <16 x float> @llvm.genx.inv.v16f32(<16 x float>)
declare <4 x float> @llvm.genx.inv.v4f32(<4 x float>)
