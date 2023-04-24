;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -mcpu=Gen9 -march=genx64 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define <16 x i32> @test(<16 x i8> %op0, <16 x i16> %op1) {
; CHECK: [[ZE0:%[^ ]+]] = zext <16 x i8> %op0 to <16 x i32>
; CHECK: [[ZE1:%[^ ]+]] = zext <16 x i16> %op1 to <16 x i32>
; CHECK: [[MULZE:%[^ ]+]] = zext <16 x i8> %op0 to <16 x i16>

; CHECK: [[MUL:%[^ ]+]] = call <16 x i32>
; CHECK-SAME: @llvm.genx.uumul
; CHECK-SAME: <16 x i16> [[MULZE]]
; CHECK-SAME: <16 x i16> %op1

; CHECK: [[MAD:%[^ ]+]] = call <16 x i32>
; CHECK-SAME: @llvm.genx.ssmad
; CHECK-SAME: <16 x i32> [[ZE0]]
; CHECK-SAME: <16 x i32> [[ZE1]]
; CHECK-SAME: <16 x i32> [[MUL]]

; CHECK: ret <16 x i32> [[MAD]]

  %ze0 = zext <16 x i8> %op0 to <16 x i32>
  %ze1 = zext <16 x i16> %op1 to <16 x i32>
  %mul = mul nuw nsw <16 x i32> %ze0, %ze1
  %fut.mad = add nuw nsw <16 x i32> %mul, %mul
  ret <16 x i32> %fut.mad
}
