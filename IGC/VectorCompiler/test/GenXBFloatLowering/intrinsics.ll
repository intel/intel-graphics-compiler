;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXBFloatLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <8 x bfloat> @llvm.genx.sat.v8bf16(<8 x bfloat>)

declare <8 x bfloat> @llvm.cos.v8bf16(<8 x bfloat>)
declare <8 x bfloat> @llvm.exp2.v8bf16(<8 x bfloat>)
declare <8 x bfloat> @llvm.fabs.v8bf16(<8 x bfloat>)
declare <8 x bfloat> @llvm.log2.v8bf16(<8 x bfloat>)
declare <8 x bfloat> @llvm.sin.v8bf16(<8 x bfloat>)
declare <8 x bfloat> @llvm.sqrt.v8bf16(<8 x bfloat>)

declare <8 x bfloat> @llvm.maximum.v8bf16(<8 x bfloat>, <8 x bfloat>)
declare <8 x bfloat> @llvm.maxnum.v8bf16(<8 x bfloat>, <8 x bfloat>)
declare <8 x bfloat> @llvm.minimum.v8bf16(<8 x bfloat>, <8 x bfloat>)
declare <8 x bfloat> @llvm.minnum.v8bf16(<8 x bfloat>, <8 x bfloat>)
declare <8 x bfloat> @llvm.pow.v8bf16(<8 x bfloat>, <8 x bfloat>)

declare <8 x bfloat> @llvm.fma.v8bf16(<8 x bfloat>, <8 x bfloat>, <8 x bfloat>)
declare <8 x bfloat> @llvm.fmuladd.v8bf16(<8 x bfloat>, <8 x bfloat>, <8 x bfloat>)

declare <8 x i32> @llvm.fptosi.sat.v8i32.v8bf16(<8 x bfloat>)
declare <8 x i32> @llvm.fptoui.sat.v8i32.v8bf16(<8 x bfloat>)

; CHECK-LABEL: test_sat
define <8 x bfloat> @test_sat(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.genx.sat.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.genx.sat.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_cos
define <8 x bfloat> @test_cos(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call fast <8 x float> @llvm.cos.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call fast <8 x bfloat> @llvm.cos.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_exp2
define <8 x bfloat> @test_exp2(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.exp2.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.exp2.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_fabs
define <8 x bfloat> @test_fabs(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.fabs.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.fabs.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_log2
define <8 x bfloat> @test_log2(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call afn <8 x float> @llvm.log2.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call afn <8 x bfloat> @llvm.log2.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_sin
define <8 x bfloat> @test_sin(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.sin.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.sin.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_sqrt
define <8 x bfloat> @test_sqrt(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.sqrt.v8f32(<8 x float> [[EXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.sqrt.v8bf16(<8 x bfloat> %src)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_maximum
define <8 x bfloat> @test_maximum(<8 x bfloat> %a, <8 x bfloat> %b) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.maximum.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.maximum.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_maxnum
define <8 x bfloat> @test_maxnum(<8 x bfloat> %a, <8 x bfloat> %b) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.maxnum.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.maxnum.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_minimum
define <8 x bfloat> @test_minimum(<8 x bfloat> %a, <8 x bfloat> %b) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.minimum.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.minimum.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_minnum
define <8 x bfloat> @test_minnum(<8 x bfloat> %a, <8 x bfloat> %b) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.minnum.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.minnum.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_pow
define <8 x bfloat> @test_pow(<8 x bfloat> %a, <8 x bfloat> %b) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.pow.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.pow.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_fma
define <8 x bfloat> @test_fma(<8 x bfloat> %a, <8 x bfloat> %b, <8 x bfloat> %c) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[CEXT:%[^ ]+]] = fpext <8 x bfloat> %c to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.fma.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]], <8 x float> [[CEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.fma.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b, <8 x bfloat> %c)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_fmuladd
define <8 x bfloat> @test_fmuladd(<8 x bfloat> %a, <8 x bfloat> %b, <8 x bfloat> %c) {
  ; CHECK: [[AEXT:%[^ ]+]] = fpext <8 x bfloat> %a to <8 x float>
  ; CHECK: [[BEXT:%[^ ]+]] = fpext <8 x bfloat> %b to <8 x float>
  ; CHECK: [[CEXT:%[^ ]+]] = fpext <8 x bfloat> %c to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x float> @llvm.fmuladd.v8f32(<8 x float> [[AEXT]], <8 x float> [[BEXT]], <8 x float> [[CEXT]])
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <8 x float> [[RES]] to <8 x bfloat>
  ; CHECK: ret <8 x bfloat> [[TRUNC]]
  %res = call <8 x bfloat> @llvm.fmuladd.v8bf16(<8 x bfloat> %a, <8 x bfloat> %b, <8 x bfloat> %c)
  ret <8 x bfloat> %res
}

; CHECK-LABEL: test_fptosi_sat
define <8 x i32> @test_fptosi_sat(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x i32> @llvm.fptosi.sat.v8i32.v8f32(<8 x float> [[EXT]])
  ; CHECK: ret <8 x i32> [[RES]]
  %res = call <8 x i32> @llvm.fptosi.sat.v8i32.v8bf16(<8 x bfloat> %src)
  ret <8 x i32> %res
}

; CHECK-LABEL: test_fptoui_sat
define <8 x i32> @test_fptoui_sat(<8 x bfloat> %src) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <8 x bfloat> %src to <8 x float>
  ; CHECK: [[RES:%[^ ]+]] = call <8 x i32> @llvm.fptoui.sat.v8i32.v8f32(<8 x float> [[EXT]])
  ; CHECK: ret <8 x i32> [[RES]]
  %res = call <8 x i32> @llvm.fptoui.sat.v8i32.v8bf16(<8 x bfloat> %src)
  ret <8 x i32> %res
}
