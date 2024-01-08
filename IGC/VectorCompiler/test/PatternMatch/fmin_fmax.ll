;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_fmax_mix
define <4 x i64> @test_fmax_mix(<4 x double> %lhs, <4 x double> %rhs) {
  ; CHECK: [[MAX:%[^ ]+]] = call <4 x double> @llvm.maxnum.v4f64(<4 x double> %lhs, <4 x double> %rhs)
  ; CHECK: [[RES:%[^ ]+]] = bitcast <4 x double> [[MAX]] to <4 x i64>
  ; CHECK: ret <4 x i64> [[RES]]
  %clhs = bitcast <4 x double> %lhs to <4 x i64>
  %crhs = bitcast <4 x double> %rhs to <4 x i64>
  %cmp = fcmp ogt <4 x double> %lhs, %rhs
  %sel = select <4 x i1> %cmp, <4 x i64> %clhs, <4 x i64> %crhs
  ret <4 x i64> %sel
}

; CHECK-LABEL: @test_fmin_mix
define <4 x i64> @test_fmin_mix(<4 x double> %lhs, <4 x double> %rhs) {
  ; CHECK: [[MIN:%[^ ]+]] = call <4 x double> @llvm.minnum.v4f64(<4 x double> %lhs, <4 x double> %rhs)
  ; CHECK: [[RES:%[^ ]+]] = bitcast <4 x double> [[MIN]] to <4 x i64>
  ; CHECK: ret <4 x i64> [[RES]]
  %clhs = bitcast <4 x double> %lhs to <4 x i64>
  %crhs = bitcast <4 x double> %rhs to <4 x i64>
  %cmp = fcmp olt <4 x double> %lhs, %rhs
  %sel = select <4 x i1> %cmp, <4 x i64> %clhs, <4 x i64> %crhs
  ret <4 x i64> %sel
}
