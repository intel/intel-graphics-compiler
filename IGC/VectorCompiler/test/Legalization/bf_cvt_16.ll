;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -mattr=+emulate_i64 -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x float> @llvm.vc.internal.cast.from.bf16.v32f32.v32i16(<32 x i16>)
declare <14 x float> @llvm.vc.internal.cast.from.bf16.v14f32.v14i16(<14 x i16>)
declare <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float>)
declare <14 x i16> @llvm.vc.internal.cast.to.bf16.v14i16.v14f32(<14 x float>)

define <32 x float> @test_bf_16_32(<32 x i16> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %in, i32 16, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <16 x float> @llvm.vc.internal.cast.from.bf16.v16f32.v16i16(<16 x i16> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> undef, <16 x float> [[RET1]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %in, i32 16, i32 16, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <16 x float> @llvm.vc.internal.cast.from.bf16.v16f32.v16i16(<16 x i16> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> [[JOIN1]], <16 x float> [[RET2]], i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  ; CHECK-NEXT: ret <32 x float> [[JOIN2]]
  %out = tail call <32 x float> @llvm.vc.internal.cast.from.bf16.v32f32.v32i16(<32 x i16> %in)
  ret <32 x float> %out
}

define <14 x float> @test_bf_16_14(<14 x i16> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x i16> @llvm.genx.rdregioni.v8i16.v14i16.i16(<14 x i16> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <14 x float> @llvm.genx.wrregionf.v14f32.v8f32.i16.i1(<14 x float> undef, <8 x float> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v14i16.i16(<14 x i16> %in, i32 4, i32 4, i32 1, i16 16, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <4 x float> @llvm.vc.internal.cast.from.bf16.v4f32.v4i16(<4 x i16> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <14 x float> @llvm.genx.wrregionf.v14f32.v4f32.i16.i1(<14 x float> [[JOIN1]], <4 x float> [[RET2]], i32 0, i32 4, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <2 x i16> @llvm.genx.rdregioni.v2i16.v14i16.i16(<14 x i16> %in, i32 2, i32 2, i32 1, i16 24, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <2 x float> @llvm.vc.internal.cast.from.bf16.v2f32.v2i16(<2 x i16> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <14 x float> @llvm.genx.wrregionf.v14f32.v2f32.i16.i1(<14 x float> [[JOIN2]], <2 x float> [[RET3]], i32 0, i32 2, i32 1, i16 48, i32 undef, i1 true)
  ; CHECK-NEXT: ret <14 x float> [[JOIN3]]
  %out = tail call <14 x float> @llvm.vc.internal.cast.from.bf16.v14f32.v14i16(<14 x i16> %in)
  ret <14 x float> %out
}

define <32 x i16> @test_bf_16_32h(<32 x float> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %in, i32 16, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> [[RET1]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %in, i32 16, i32 16, i32 1, i16 64, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> [[JOIN1]], <16 x i16> [[RET2]], i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT: ret <32 x i16> [[JOIN2]]
  %out = tail call <32 x i16> @llvm.vc.internal.cast.to.bf16.v32i16.v32f32(<32 x float> %in)
  ret <32 x i16> %out
}

define <14 x i16> @test_bf_16_14f(<14 x float> %in) {
  ; CHECK: [[SP1:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v14f32.i16(<14 x float> %in, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: [[RET1:%[^ ]+]] = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> [[SP1]])
  ; CHECK-NEXT: [[JOIN1:%[^ ]+]] = call <14 x i16> @llvm.genx.wrregioni.v14i16.v8i16.i16.i1(<14 x i16> undef, <8 x i16> [[RET1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP2:%[^ ]+]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v14f32.i16(<14 x float> %in, i32 4, i32 4, i32 1, i16 32, i32 undef)
  ; CHECK-NEXT: [[RET2:%[^ ]+]] = call <4 x i16> @llvm.vc.internal.cast.to.bf16.v4i16.v4f32(<4 x float> [[SP2]])
  ; CHECK-NEXT: [[JOIN2:%[^ ]+]] = call <14 x i16> @llvm.genx.wrregioni.v14i16.v4i16.i16.i1(<14 x i16> [[JOIN1]], <4 x i16> [[RET2]], i32 0, i32 4, i32 1, i16 16, i32 undef, i1 true)
  ; CHECK-NEXT: [[SP3:%[^ ]+]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v14f32.i16(<14 x float> %in, i32 2, i32 2, i32 1, i16 48, i32 undef)
  ; CHECK-NEXT: [[RET3:%[^ ]+]] = call <2 x i16> @llvm.vc.internal.cast.to.bf16.v2i16.v2f32(<2 x float> [[SP3]])
  ; CHECK-NEXT: [[JOIN3:%[^ ]+]] = call <14 x i16> @llvm.genx.wrregioni.v14i16.v2i16.i16.i1(<14 x i16> [[JOIN2]], <2 x i16> [[RET3]], i32 0, i32 2, i32 1, i16 24, i32 undef, i1 true)
  ; CHECK-NEXT: ret <14 x i16> [[JOIN3]]
  %out = tail call <14 x i16> @llvm.vc.internal.cast.to.bf16.v14i16.v14f32(<14 x float> %in)
  ret <14 x i16> %out
}
