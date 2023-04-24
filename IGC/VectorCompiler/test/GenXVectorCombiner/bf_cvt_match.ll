;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXVectorCombiner -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

declare <32 x float> @llvm.genx.wrregioni.v32f32.v16f32.i16.i1(<32 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)

declare <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float>, i32, i32, i32, i16, i32)
declare <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16>, <16 x i16>, i32, i32, i32, i16, i32, i1)

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)

declare <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float>, i32, i32, i32, i16, i32)
declare <128 x i16>  @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16>, <16 x i16>, i32, i32, i32, i16, i32, i1)

declare <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float>)
declare <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float>)

define <32 x i16> @bf_cvt_Match(<32 x float> %op0)
{
  ; CHECK: [[V1:%[^ ]+]]  = call <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float> %op0)
  %rd1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %bf1 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %rd1)
  %wr1 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %bf1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %rd2 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %bf2 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %rd2)
  %wr2 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %wr1, <16 x i16> %bf2, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  ret <32 x i16> %wr2
}
; different order - not match
define <32 x i16> @bf_cvt_notMatch(<32 x float> %op0)
{
  ; CHECK: [[V1:%[^ ]+]]  = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %rd2)
  %rd2 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %bf2 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %rd2)
  %wr2 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %bf2, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  %rd1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %op0, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %bf1 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %rd1)
  %wr1 = tail call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %wr2, <16 x i16> %bf1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ret <32 x i16> %wr2
}
; original
define <128 x i16> @bf_big(<128 x float> %max, <128 x i16> %WrSrc)
{
  ; CHECK: [[V1:%[^ ]+]] = call <128 x i16> @llvm.vc.internal.cast.to.bf16.v128i16.v128f32(
  %.regioncollapsed1684 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %1 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1684)
  %2 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %WrSrc, <16 x i16> %1, i32 16, i32 16, i32 1, i16 0, i32 16, i1 true)
  %.regioncollapsed1683 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %3 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1683)
  %4 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %2, <16 x i16> %3, i32 16, i32 16, i32 1, i16 32, i32 16, i1 true)
  %.regioncollapsed1682 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 128, i32 undef)
  %5 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1682)
  %6 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %4, <16 x i16> %5, i32 16, i32 16, i32 1, i16 64, i32 16, i1 true)
  %.regioncollapsed1681 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 192, i32 undef)
  %7 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1681)
  %8 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %6, <16 x i16> %7, i32 16, i32 16, i32 1, i16 96, i32 16, i1 true)
  %.regioncollapsed1680 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 256, i32 undef)
  %9 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1680)
  %10 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %8, <16 x i16> %9, i32 16, i32 16, i32 1, i16 128, i32 16, i1 true)
  %.regioncollapsed1679 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 320, i32 undef)
  %11 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1679)
  %12 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %10, <16 x i16> %11, i32 16, i32 16, i32 1, i16 160, i32 16, i1 true)
  %.regioncollapsed1678 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 384, i32 undef)
  %13 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1678)
  %14 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %12, <16 x i16> %13, i32 16, i32 16, i32 1, i16 192, i32 16, i1 true)
  %.regioncollapsed1677 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 448, i32 undef)
  %15 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1677)
  %16 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %14, <16 x i16> %15, i32 16, i32 16, i32 1, i16 224, i32 16, i1 true)
  ret <128 x i16> %16
}
; big some rd missed - not optimized
define <128 x i16> @bf_big_not_opt(<128 x float> %max, <128 x i16> %WrSrc)
{
  ; CHECK: [[V1:%[^ ]+]] = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(
  %.regioncollapsed1684 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %1 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1684)
  %2 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %WrSrc, <16 x i16> %1, i32 16, i32 16, i32 1, i16 0, i32 16, i1 true)
  %.regioncollapsed1683 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %3 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1683)
  %4 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %2, <16 x i16> %3, i32 16, i32 16, i32 1, i16 32, i32 16, i1 true)
  %.regioncollapsed1682 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 128, i32 undef)
  %5 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1682)
  %6 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %4, <16 x i16> %5, i32 16, i32 16, i32 1, i16 64, i32 16, i1 true)
  %.regioncollapsed1681 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 192, i32 undef)
  %7 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1681)
  %8 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %6, <16 x i16> %7, i32 16, i32 16, i32 1, i16 96, i32 16, i1 true)
  %.regioncollapsed1679 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 320, i32 undef)
  %9 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1679)
  %10 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %8, <16 x i16> %9, i32 16, i32 16, i32 1, i16 160, i32 16, i1 true)
  %.regioncollapsed1678 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 384, i32 undef)
  %11 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1678)
  %12 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %10, <16 x i16> %11, i32 16, i32 16, i32 1, i16 192, i32 16, i1 true)
  %.regioncollapsed1677 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v128f32.i16(<128 x float> %max, i32 0, i32 16, i32 1, i16 448, i32 undef)
  %13 = tail call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %.regioncollapsed1677)
  %14 = tail call <128 x i16> @llvm.genx.wrregioni.v128i16.v16i16.i16.i1(<128 x i16> %12, <16 x i16> %13, i32 16, i32 16, i32 1, i16 224, i32 16, i1 true)
  ret <128 x i16> %14
}
