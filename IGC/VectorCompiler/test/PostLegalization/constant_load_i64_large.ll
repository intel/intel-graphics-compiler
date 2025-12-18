;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Xe3P -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

declare void @llvm.genx.lsc.store.stateless.v1i1.i64.v2i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, i64, <2 x i64>, i32)

define void @test(i64 %addr) {
  ; CHECK: %[[CONST:[^ ]+]] = call <1 x i64> @llvm.genx.constanti.v1i64(<1 x i64> <i64 4656722015785320448>)
  ; CHECK: %[[SPLAT:[^ ]+]] = call <2 x i64> @llvm.genx.rdregioni.v2i64.v1i64.i16(<1 x i64> %[[CONST]], i32 0, i32 2, i32 0, i16 0, i32 undef)
  ; CHECK: %[[VALUE:[^ ]+]] = call <2 x i64> @llvm.genx.wrconstregion.v2i64.v1i64.i16.i1(<2 x i64> %[[SPLAT]], <1 x i64> <i64 4656722015774834688>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call void @llvm.genx.lsc.store.stateless.v1i1.i64.v2i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i64 %addr, <2 x i64> %[[VALUE]], i32 0)
  call void @llvm.genx.lsc.store.stateless.v1i1.i64.v2i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i64 %addr, <2 x i64> <i64 4656722015774834688, i64 4656722015785320448>, i32 0)
  ret void
}
