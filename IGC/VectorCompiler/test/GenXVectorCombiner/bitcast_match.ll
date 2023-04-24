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

declare <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)

declare <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float>, i32, i32, i32, i16, i32)
declare <32 x half> @llvm.genx.wrregionf.v32f16.v16f16.i16.i1(<32 x half>, <16 x half>, i32, i32, i32, i16, i32, i1)

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)
declare <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16>, <16 x i16>, i32, i32, i32, i16, i32, i1)

declare <16 x half> @llvm.genx.bf.cvt.v16f16.v16f32(<16 x float>)
declare <32 x half> @llvm.genx.bf.cvt.v32f16.v32f32(<32 x float>)

define <32 x i32> @bitcast_Match(<32 x float> %op0)
{
  ; CHECK: [[V1:%[^ ]+]]  = bitcast <32 x float> %op0 to <32 x i32>
  %rd1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %cast1 = bitcast <16 x float> %rd1 to <16 x i32>
  %wr1 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %cast1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %rd2 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %cast2= bitcast <16 x float> %rd2 to <16 x i32>
  %wr2 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %wr1, <16 x i32> %cast2, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  ret <32 x i32> %wr2
}
define <32 x i16> @trunc_Match(<32 x i32> %op0)
{
  ; CHECK: [[V1:%[^ ]+]]  = trunc <32 x i32> %op0 to <32 x i16>
  %rd1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %cast1 = trunc <16 x i32> %rd1 to <16 x i16>
  %wr1 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %cast1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %rd2 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %cast2 = trunc <16 x i32> %rd2 to <16 x i16>
  %wr2 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %wr1, <16 x i16> %cast2, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  ret <32 x i16> %wr2
}
