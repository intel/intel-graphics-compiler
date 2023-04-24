;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeLP -mtriple=spir64 -S < %s | FileCheck %s

;; Test legalization of constants as return values (constant loader).

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare <8 x i64> @llvm.genx.constanti.v8i64(<8 x i64>)
; CHECK-LABEL: @legalize_load_emu64
; CHECK-NEXT: [[CI32:%.+]] = call <[[ET:8 x i32]]> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT: [[SCALED:%.+]] = mul <[[ET]]> [[CI32]], <i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4>
; CHECK-NEXT: [[SIGN_PART:%.+]] = ashr <[[ET]]> [[SCALED]], <i32 31, i32 31, i32 31, i32 31, i32 31, i32 31, i32 31, i32 31>
; CHECK-NEXT: [[P_JOIN:%.+]] = call <[[CT:16 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[SCALED]], [[WR_ARGS:i32 0, i32 8, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%.+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[SIGN_PART]], [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTED:%.+]] = bitcast <[[CT]]> [[JOINED]] to <8 x i64>
; CHECK-NEXT: ret <8 x i64> [[CASTED]]
define <8 x i64> @legalize_load_emu64() {
  %constant = call <8 x i64> @llvm.genx.constanti.v8i64(<8 x i64> <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28>)
  ret <8 x i64> %constant
}

