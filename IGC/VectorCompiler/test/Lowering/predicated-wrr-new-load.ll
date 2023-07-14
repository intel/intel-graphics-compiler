;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s


define internal spir_func <32 x i64> @test_predicate(<32 x i1> %pred,  <32 x i64> %addrs) {
; CHECK: [[CALL:%.*]] = tail call <32 x i64> @llvm.genx.lsc.load.stateless.v32i64.v32i1.v32i64
; CHECK-NEXT: [[PREDICATED_WRR:%.*]] = call <32 x i64> @llvm.genx.wrregioni.v32i64.v32i64.i16.v32i1(<32 x i64> zeroinitializer, <32 x i64> [[CALL]], i32 0, i32 32, i32 1, i16 0, i32 undef, <32 x i1> %pred)
; CHECK-NEXT:  ret <32 x i64> [[PREDICATED_WRR]]
 %res = tail call <32 x i64> @llvm.genx.lsc.load.stateless.v32i64.v32i1.v32i64(<32 x i1> %pred, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <32 x i64> %addrs, i32 0)
 %sel = select <32 x i1> %pred, <32 x i64> %res, <32 x i64> zeroinitializer
 ret <32 x i64> %sel
}

declare <32 x i64> @llvm.genx.lsc.load.stateless.v32i64.v32i1.v32i64(<32 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <32 x i64>, i32)
