;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <128 x i16> @llvm.genx.bdpas.v128i16.v128i16.v128i32.v64i32.v16i8.v8i8(<128 x i16>, <128 x i32>, <64 x i32>, <16 x i8>, <8 x i8>, i32, i32, i32, i32)

; CHECK-LABEL: @test(
define <128 x i16> @test(<128 x i16> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale) {
; CHECK: [[ACC:%.*]] = bitcast <128 x i16> %acc to <128 x bfloat>
; CHECK: [[DPAS:%.*]] = call <128 x bfloat> @llvm.genx.bdpas.v128bf16.v128bf16.v128i32.v64i32.v16i8.v8i8(<128 x bfloat> [[ACC]], <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
; CHECK: [[RES:%.*]] = bitcast <128 x bfloat> [[DPAS]] to <128 x i16>
; CHECK: ret <128 x i16> [[RES]]
  %res = call <128 x i16> @llvm.genx.bdpas.v128i16.v128i16.v128i32.v64i32.v16i8.v8i8(<128 x i16> %acc, <128 x i32> %b, <64 x i32> %a, <16 x i8> %bscale, <8 x i8> %ascale, i32 9, i32 9, i32 8, i32 8)
  ret <128 x i16> %res
}
