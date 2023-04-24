;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define <4 x i8> @test_xor(<4 x i8> %op)  {
  ; CHECK: %.extend.mask.op = bitcast <4 x i8> %op to <1 x i32>
  ; CHECK: %1 = xor <1 x i32> %.extend.mask.op, <i32 -1>
  ; CHECK: %.extend.mask.trunc = bitcast <1 x i32> %1 to <4 x i8>
  ; CHECK: ret <4 x i8> %.extend.mask.trunc
  %1 = xor <4 x i8> %op, <i8 -1, i8 -1, i8 -1, i8 -1>
  ret <4 x i8> %1
}

define <4 x i8> @test_or(<4 x i8> %op)  {
  ; CHECK: %.extend.mask.op = bitcast <4 x i8> %op to <1 x i32>
  ; CHECK: %1 = or <1 x i32> %.extend.mask.op, <i32 1>
  ; CHECK: %.extend.mask.trunc = bitcast <1 x i32> %1 to <4 x i8>
  ; CHECK: ret <4 x i8> %.extend.mask.trunc
  %1 = or <4 x i8> %op, <i8 1, i8 0, i8 0, i8 0>
  ret <4 x i8> %1
}

define <4 x i8> @test_and(<4 x i8> %op)  {
  ; CHECK: %.extend.mask.op = bitcast <4 x i8> %op to <1 x i32>
  ; CHECK: %1 = and <1 x i32> %.extend.mask.op, <i32 1>
  ; CHECK: %.extend.mask.trunc = bitcast <1 x i32> %1 to <4 x i8>
  ; CHECK: ret <4 x i8> %.extend.mask.trunc
  %1 = and <4 x i8> %op, <i8 1, i8 0, i8 0, i8 0>
  ret <4 x i8> %1
}
