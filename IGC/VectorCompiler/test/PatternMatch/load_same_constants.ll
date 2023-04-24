;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define dllexport spir_kernel void @test(<16 x i32>* %val, i1 %cond) {
; CHECK: [[CONSTANT:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 1>)
; CHECK-NEXT: [[CONSTANT_SPLAT:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> [[CONSTANT]], i32 0, i32 16, i32 0, i16 0, i32 undef)
  br i1 %cond, label %bb1, label %bb2

bb1:
  br label %bb3

bb2:
  br label %bb3

bb3:
; CHECK: bb3:
; CHECK-NEXT: store <16 x i32> [[CONSTANT_SPLAT]], <16 x i32>* %val
  %.sink101 = phi <16 x i32> [ <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>,  %bb1 ], [ <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, %bb2 ]
  store <16 x i32> %.sink101,  <16 x i32>* %val
  ret void
}

