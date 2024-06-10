;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <128 x i16> @llvm.genx.dpas.v128i16.v128i32.v64i32(<128 x i16>, <128 x i32>, <64 x i32>, i32)
declare <128 x i16> @llvm.genx.dpas.nosrc0.v128i16.v128i32.v64i32(<128 x i32>, <64 x i32>, i32)
declare <128 x i16> @llvm.genx.dpas2.v128i16.v128i16.v128i32.v64i32(<128 x i16>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32)

; CHECK-LABEL: @test1(
define <128 x i16> @test1(<128 x i16> %acc, <64 x i32> %a, <128 x i32> %b) {
; CHECK: [[ACC:%.*]] = bitcast <128 x i16> %acc to <128 x bfloat>
; CHECK: [[DPAS:%.*]] = call <128 x bfloat> @llvm.genx.dpas2.v128bf16.v128bf16.v128i32.v64i32(<128 x bfloat> [[ACC]], <128 x i32> %b, <64 x i32> %a, i32 9, i32 9, i32 8, i32 8, i32 0, i32 0)
; CHECK: [[RES:%.*]] = bitcast <128 x bfloat> [[DPAS]] to <128 x i16>
; CHECK: ret <128 x i16> [[RES]]
  %res = call <128 x i16> @llvm.genx.dpas2.v128i16.v128i16.v128i32.v64i32(<128 x i16> %acc, <128 x i32> %b, <64 x i32> %a, i32 9, i32 9, i32 8, i32 8, i32 0, i32 0)
  ret <128 x i16> %res
}

; CHECK-LABEL: @test2(
define <128 x i16> @test2(<64 x i32> %a, <128 x i32> %b) {
; CHECK: [[DPAS:%.*]] = call <128 x bfloat> @llvm.genx.dpas.nosrc0.v128bf16.v128i32.v64i32(<128 x i32> %b, <64 x i32> %a, i32 134744329)
; CHECK: [[RES:%.*]] = bitcast <128 x bfloat> [[DPAS]] to <128 x i16>
; CHECK: ret <128 x i16> [[RES]]
  %res = call <128 x i16> @llvm.genx.dpas.nosrc0.v128i16.v128i32.v64i32(<128 x i32> %b, <64 x i32> %a, i32 134744329)
  ret <128 x i16> %res
}

; CHECK-LABEL: @test3(
define <128 x i16> @test3(<128 x i16> %acc, <64 x i32> %a, <128 x i32> %b) {
; CHECK: [[ACC:%.*]] = bitcast <128 x i16> %acc to <128 x bfloat>
; CHECK: [[DPAS:%.*]] = call <128 x bfloat> @llvm.genx.dpas.v128bf16.v128i32.v64i32(<128 x bfloat> [[ACC]], <128 x i32> %b, <64 x i32> %a, i32 134744329)
; CHECK: [[RES:%.*]] = bitcast <128 x bfloat> [[DPAS]] to <128 x i16>
; CHECK: ret <128 x i16> [[RES]]
  %res = call <128 x i16> @llvm.genx.dpas.v128i16.v128i32.v64i32(<128 x i16> %acc, <128 x i32> %b, <64 x i32> %a, i32 134744329)
  ret <128 x i16> %res
}
