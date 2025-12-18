;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Xe3P -mtriple=spir64 -S < %s | FileCheck %s

declare <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16f16.v16i8(<16 x half>, <16 x i8>)
declare <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16bf16.v16i8(<16 x bfloat>, <16 x i8>)
declare <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16f16.v16i8(<16 x half>, <16 x i8>)
declare <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16bf16.v16i8(<16 x bfloat>, <16 x i8>)

; CHECK-LABEL: @test_f16_to_bf8
define <16 x i8> @test_f16_to_bf8(<16 x half> %src) {
; CHECK: [[CONSTI:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
; CHECK: [[SPLAT:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[CONSTI]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i32> [[SPLAT]] to <16 x i8>
; CHECK: [[RES:%[^ ]+]] = call <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16f16.v16i8(<16 x half> %src, <16 x i8> [[CAST]])
; CHECK: ret <16 x i8> [[RES]]
  %res = call <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16f16.v16i8(<16 x half> %src, <16 x i8> zeroinitializer)
  ret <16 x i8> %res
}

; CHECK-LABEL: @test_f16_to_hf8
define <16 x i8> @test_f16_to_hf8(<16 x half> %src) {
; CHECK: [[CONSTI:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
; CHECK: [[SPLAT:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[CONSTI]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i32> [[SPLAT]] to <16 x i8>
; CHECK: [[RES:%[^ ]+]] = call <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16f16.v16i8(<16 x half> %src, <16 x i8> [[CAST]])
; CHECK: ret <16 x i8> [[RES]]
  %res = call <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16f16.v16i8(<16 x half> %src, <16 x i8> zeroinitializer)
  ret <16 x i8> %res
}

; CHECK-LABEL: @test_bf16_to_bf8
define <16 x i8> @test_bf16_to_bf8(<16 x bfloat> %src) {
; CHECK: [[CONSTI:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
; CHECK: [[SPLAT:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[CONSTI]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i32> [[SPLAT]] to <16 x i8>
; CHECK: [[RES:%[^ ]+]] = call <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16bf16.v16i8(<16 x bfloat> %src, <16 x i8> [[CAST]])
; CHECK: ret <16 x i8> [[RES]]
  %res = call <16 x i8> @llvm.vc.internal.stochastic.round.to.bf8.v16i8.v16bf16.v16i8(<16 x bfloat> %src, <16 x i8> zeroinitializer)
  ret <16 x i8> %res
}

; CHECK-LABEL: @test_bf16_to_hf8
define <16 x i8> @test_bf16_to_hf8(<16 x bfloat> %src) {
; CHECK: [[CONSTI:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
; CHECK: [[SPLAT:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[CONSTI]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i32> [[SPLAT]] to <16 x i8>
; CHECK: [[RES:%[^ ]+]] = call <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16bf16.v16i8(<16 x bfloat> %src, <16 x i8> [[CAST]])
; CHECK: ret <16 x i8> [[RES]]
  %res = call <16 x i8> @llvm.vc.internal.stochastic.round.to.hf8.v16i8.v16bf16.v16i8(<16 x bfloat> %src, <16 x i8> zeroinitializer)
  ret <16 x i8> %res
}
