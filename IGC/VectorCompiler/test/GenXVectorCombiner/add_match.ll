;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXVectorCombiner -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)
declare <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16>, <16 x i16>, i32, i32, i32, i16, i32, i1)

declare <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16>, i32, i32, i32, i16, i32)
declare <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16>, <8 x i16>, i32, i32, i32, i16, i32, i1)
declare <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.i1(<16 x i16>, <8 x i16>, i32, i32, i32, i16, i32, i1)

declare <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float>, i32, i32, i32, i16, i32)
declare <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)

declare <16 x half> @llvm.genx.rdregionf.v16f16.v32f16.i16(<32 x float>, i32, i32, i32, i16, i32)
declare <32 x half> @llvm.genx.wrregionf.v32f16.v16f16.i16.i1(<32 x half>, <16 x half>, i32, i32, i32, i16, i32, i1)

; optimize
define <32 x float> @test_match_v32f32(<32 x float> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = fadd <32 x float> %op0, <float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01>
  %1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = fadd <16 x float> %1, <float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01>
  %3 = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> undef, <16 x float> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %5 = fadd <16 x float> %4, <float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01, float 1.900000e+01>
  %6 = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> %3, <16 x float> %5, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)

  ret <32 x float> %6
}
; optimize
define <32 x i32> @test_match_v32i32(<32 x i32> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <32 x i32> %op0, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = add <16 x i32> %1, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %3 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %5 = add <16 x i32> %4, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %6 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %3, <16 x i32> %5, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)

  ret <32 x i32> %6
}

; different constants - no optimize
define <32 x i32> @test_nomatch_v32i32(<32 x i32> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <16 x i32> %1, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = add <16 x i32> %1, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %3 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %5 = add <16 x i32> %4, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 10>
  %6 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %3, <16 x i32> %5, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)

  ret <32 x i32> %6
}
define <32 x i16> @test_match_v32i16(<32 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <32 x i16> %op0, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %1 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = add <16 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %3 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 32, i32 undef)
  %5 = add <16 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %6 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %3, <16 x i16> %5, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)

  ret <32 x i16> %6
}
; different constants - no optimize
define <32 x i16> @test_nomatch_v32i16(<32 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <16 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %1 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = add <16 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %3 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 32, i32 undef)
  %5 = add <16 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 10>
  %6 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %3, <16 x i16> %5, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)

  ret <32 x i16> %6
}
; not constant - no optimize
define <32 x i16> @test_nomatch_v32i16_noconst(<32 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <16 x i16> %1, %1
  %1 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %2 = add <16 x i16> %1, %1
  %3 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %2, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %op0, i32 0, i32 16, i32 1, i16 32, i32 undef)
  %5 = add <16 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 10>
  %6 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %3, <16 x i16> %5, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)

  ret <32 x i16> %6
}

; smaller width - optimize
define <16 x i16> @test_match_v16i16(<16 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <16 x i16> %op0,
  %1 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %2 = add <8 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %3 = tail call <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.i1(<16 x i16> undef, <8 x i16> %2, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 16, i32 undef)
  %5 = add <8 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %6 = tail call <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.i1(<16 x i16> %3, <8 x i16> %5, i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)

  ret <16 x i16> %6
}

; write to different element count - not optimize
define <32 x i16> @test_not_match_v16i16(<16 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <8 x i16>
  %1 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %2 = add <8 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %3 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> undef, <8 x i16> %2, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %4 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 16, i32 undef)
  %5 = add <8 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %6 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> %3, <8 x i16> %5, i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)

  ret <32 x i16> %6
}
; mixed - not optimize

define <16 x i16> @test_mixed_not_match_v16i16(<16 x i16> %op0)  {
  ;CHECK: [[V1:%[^ ]+]]  = add <8 x i16>
  %1 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %2 = add <8 x i16> %1, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %3 = tail call <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.i1(<16 x i16> undef, <8 x i16> %2, i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)
  %4 = tail call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %op0, i32 0, i32 8, i32 1, i16 16, i32 undef)
  %5 = add <8 x i16> %4, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  %6 = tail call <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.i1(<16 x i16> %3, <8 x i16> %5, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)

  ret <16 x i16> %6
}
