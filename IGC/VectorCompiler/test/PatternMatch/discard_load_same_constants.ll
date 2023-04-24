;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define dllexport spir_kernel void @test(<16 x i32>* %val, i1 %cond) {
  br i1 %cond, label %bb1, label %bb2

bb1:
; CHECK: bb1:
; CHECK-NEXT: [[CONSTANT1:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 1>)
  br label %bb3

bb2:
; CHECK: bb2:
; CHECK-NEXT: [[CONSTANT2:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
  br label %bb3

bb3:
  %.sink101 = phi <16 x i32> [ <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>,  %bb1 ], [ zeroinitializer, %bb2 ]
  store <16 x i32> %.sink101,  <16 x i32>* %val
  ret void
}

