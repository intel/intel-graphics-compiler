;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncLiveElements -print-live-elements-info \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -disable-output -S < %s | FileCheck %s

declare {<8 x i32>, <8 x i32>} @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.wrregioni.v8i64.v4i64.i16.i1(<8 x i64>, <4 x i64>, i32, i32, i32, i16, i32, i1)
declare i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16)

; CHECK: Live elements:
; CHECK-DAG: <8 x i32> %arg1: {11111111}
; CHECK-DAG: <8 x i32> %arg2: {11111111}
; CHECK-DAG: %1 = call { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32> %arg1, <8 x i32> %arg2): {11111111, 11111111}
; CHECK-DAG: %2 = insertvalue { <8 x i32>, <8 x i32> } %1, <8 x i32> %arg3, 1: {11111111, 00000000}
; CHECK-DAG: %3 = extractvalue { <8 x i32>, <8 x i32> } %2, 0: {11111111}
; CHECK-DAG: %4 = bitcast <8 x i32> %3 to <4 x i64>: {1111}
; CHECK-DAG: %5 = call <8 x i64> @llvm.genx.wrregioni.v8i64.v4i64.i16.i1(<8 x i64> undef, <4 x i64> %4, i32 0, i32 4, i32 2, i16 0, i32 undef, i1 true): {11111111}
; CHECK-DAG: %6 = bitcast <8 x i64> %5 to <16 x i32>: {0000100000000000}
; CHECK-DAG: %7 = call i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32> %6, i32 0, i32 1, i32 0, i16 16): {1}
; CHECK-DAG: ret i32 %7: {}

define i32 @test(<8 x i32> %arg1, <8 x i32> %arg2, <8 x i32> %arg3) {
  %1 = call {<8 x i32>, <8 x i32>} @llvm.genx.addc.v8i32.v8i32(<8 x i32> %arg1, <8 x i32> %arg2)
  %2 = insertvalue {<8 x i32>, <8 x i32>} %1, <8 x i32> %arg3, 1
  %3 = extractvalue {<8 x i32>, <8 x i32>} %2, 0
  %4 = bitcast <8 x i32> %3 to <4 x i64>
  %5 = call <8 x i64> @llvm.genx.wrregioni.v8i64.v4i64.i16.i1(<8 x i64> undef, <4 x i64> %4, i32 0, i32 4, i32 2, i16 0, i32 undef, i1 true)
  %6 = bitcast <8 x i64> %5 to <16 x i32>
  %7 = call i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32> %6, i32 0, i32 1, i32 0, i16 16)
  ret i32 %7
}
