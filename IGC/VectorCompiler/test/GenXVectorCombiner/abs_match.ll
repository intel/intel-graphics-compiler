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

declare <16 x float> @llvm.genx.absf.v16f32.v16f32(<16 x float>)
declare <32 x float> @llvm.genx.absf.v32f32.v32f32(<32 x float>)

declare <16 x i32> @llvm.genx.absi.v16i32.v16i32(<16 x i32>)
declare <32 x i32> @llvm.genx.absi.v32i32.v32i32(<32 x i32>)

define <32 x float> @absf_Match(<32 x float> %op0)
{
  ; CHECK: [[V1:%[^ ]+]] = call <32 x float> @llvm.genx.absf.v32f32(<32 x float> %op0)

  %rd1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
     %abs1 = tail call <16 x float> @llvm.genx.absf.v16f32.v16f32(<16 x float> %rd1)
     %wr1 = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> undef, <16 x float> %abs1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
     %rd2 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
     %abs2 = tail call <16 x float> @llvm.genx.absf.v16f32.v16f32(<16 x float> %rd2)
     %wr2 = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> %wr1, <16 x float> %abs2, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
     ret <32 x float> %wr2
}
define <32 x i32> @absi_Match(<32 x i32> %op0)
{
  ; CHECK: [[V1:%[^ ]+]] = call <32 x i32> @llvm.genx.absi.v32i32(<32 x i32> %op0)
  %rd1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
     %abs1 = tail call <16 x i32> @llvm.genx.absi.v16i32.v16i32(<16 x i32> %rd1)
     %wr1 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %abs1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
     %rd2 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
     %abs2 = tail call <16 x i32> @llvm.genx.absi.v16i32.v16i32(<16 x i32> %rd2)
     %wr2 = tail call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %wr1, <16 x i32> %abs2, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
     ret <32 x i32> %wr2
}
