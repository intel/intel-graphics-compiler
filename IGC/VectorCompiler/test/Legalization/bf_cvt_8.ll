;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x float> @llvm.vc.internal.cast.from.bf16.v32f32.v32i16(<32 x i16>)
declare <17 x float> @llvm.vc.internal.cast.from.bf16.v17f32.v17i16(<17 x i16>)
declare <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float>)
declare <17 x i16> @llvm.vc.internal.cast.to.bf16.v17i16.v17f32(<17 x float>)

define <32 x float> @test_bf_8_32(<32 x i16> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v32i16.i16(<32 x i16> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v8f32.i16.i1(<32 x float> undef, <8 x float> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v32i16.i16(<32 x i16> %in, i32 8, i32 8, i32 1, i16 16, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v8f32.i16.i1(<32 x float> [[JOIN1]], <8 x float> [[RET2]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v32i16.i16(<32 x i16> %in, i32 8, i32 8, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v8f32.i16.i1(<32 x float> [[JOIN2]], <8 x float> [[RET3]], i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP4:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v32i16.i16(<32 x i16> %in, i32 8, i32 8, i32 1, i16 48, i32 undef)
  ; CHECK-NEXT: [[RET4:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP4]])
  ; CHECK-NEXT: [[JOIN4:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v8f32.i16.i1(<32 x float> [[JOIN3]], <8 x float> [[RET4]], i32 0, i32 8, i32 1, i16 96, i32 undef, i1 true)
  ; CHECK-NEXT: ret <32 x float> [[JOIN4]]
  %out = tail call <32 x float> @llvm.vc.internal.cast.from.bf16.v32f32.v32i16(<32 x i16> %in)
  ret <32 x float> %out
}

define <17 x float> @test_bf_8_17(<17 x i16> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v17i16.i16(<17 x i16> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <17 x float> @llvm.genx.wrregionf.v17f32.v8f32.i16.i1(<17 x float> undef, <8 x float> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v17i16.i16(<17 x i16> %in, i32 8, i32 8, i32 1, i16 16, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <17 x float> @llvm.genx.wrregionf.v17f32.v8f32.i16.i1(<17 x float> %.split0.join0, <8 x float> [[RET2]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <1 x i16> @llvm.genx.rdregioni.v1i16.v17i16.i16(<17 x i16> %in, i32 1, i32 1, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <1 x float> @llvm.vc.internal.cast.from.bf16.v1f32.v1i16(<1 x i16> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <17 x float> @llvm.genx.wrregionf.v17f32.v1f32.i16.i1(<17 x float> %.split8.join8, <1 x float> [[RET3]], i32 0, i32 1, i32 1, i16 64, i32 undef, i1 true)
  ; CHECK-NEXT: ret <17 x float> [[JOIN3]]
  %out = tail call <17 x float> @llvm.vc.internal.cast.from.bf16.v17f32.v17i16(<17 x i16> %in)
  ret <17 x float> %out
}


define <32 x i16> @test_bf_8_32h(<32 x float> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v32f32.i16(<32 x float> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> undef, <8 x i16> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v32f32.i16(<32 x float> %in, i32 8, i32 8, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> [[JOIN1]], <8 x i16> [[RET2]], i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v32f32.i16(<32 x float> %in, i32 8, i32 8, i32 1, i16 64, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> [[JOIN2]], <8 x i16> [[RET3]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP4:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v32f32.i16(<32 x float> %in, i32 8, i32 8, i32 1, i16 96, i32 undef)
  ; CHECK-NEXT: [[RET4:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP4]])
  ; CHECK-NEXT: [[JOIN4:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v8i16.i16.i1(<32 x i16> [[JOIN3]], <8 x i16> [[RET4]], i32 0, i32 8, i32 1, i16 48, i32 undef, i1 true)
  ; CHECK-NEXT: ret <32 x i16> [[JOIN4]]
  %out = tail call <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float> %in)
  ret <32 x i16> %out
}

define <17 x i16> @test_bf_8_17h(<17 x float> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v17f32.i16(<17 x float> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <17 x i16> @llvm.genx.wrregioni.v17i16.v8i16.i16.i1(<17 x i16> undef, <8 x i16> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v17f32.i16(<17 x float> %in, i32 8, i32 8, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <17 x i16> @llvm.genx.wrregioni.v17i16.v8i16.i16.i1(<17 x i16> %.split0.join0, <8 x i16> [[RET2]], i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v17f32.i16(<17 x float> %in, i32 1, i32 1, i32 1, i16 64, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <1 x i16> @llvm.vc.internal.cast.to.bf16.v1i16.v1f32(<1 x float> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <17 x i16> @llvm.genx.wrregioni.v17i16.v1i16.i16.i1(<17 x i16> %.split8.join8, <1 x i16> [[RET3]], i32 0, i32 1, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: ret <17 x i16> [[JOIN3]]
  %out = tail call <17 x i16> @llvm.vc.internal.cast.to.bf16.v17i16.v17f32(<17 x float> %in)
  ret <17 x i16> %out
}
