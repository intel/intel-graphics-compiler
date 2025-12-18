;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i64> @llvm.genx.uuadd.sat.v16i64.v16i64(<16 x i64>, <16 x i64>)

; CHECK-LABEL: @Test
define <16 x i64> @Test(<16 x i16> %0, <16 x i16> %1, <16 x i64> %2) {
  ;CHECK: [[ZXT1:%[^ ]+]] = zext <16 x i16> %0 to <16 x i64>
  ;CHECK-NEXT: [[ZXT2:%[^ ]+]] = zext <16 x i16> %1 to <16 x i64>
  ;CHECK-NEXT: [[MUL:%[^ ]+]] = mul nuw <16 x i64> [[ZXT1]], [[ZXT2]]
  ;CHECK-NEXT: [[CALL:%[^ ]+]] = tail call <16 x i64> @llvm.genx.uuadd.sat.v16i64.v16i64(<16 x i64> [[MUL]], <16 x i64> %2)
  ;CHECK-NEXT: ret <16 x i64> [[CALL]]

  %conv8 = zext <16 x i16> %0 to <16 x i64>
  %conv14 = zext <16 x i16> %1 to <16 x i64>
  %mul = mul nuw <16 x i64> %conv8, %conv14
  %call = tail call <16 x i64> @llvm.genx.uuadd.sat.v16i64.v16i64(<16 x i64> %mul, <16 x i64> %2)
  ret <16 x i64> %call
}
