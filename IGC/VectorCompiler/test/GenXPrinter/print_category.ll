;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Check that value category category is printed.

; RUN: llc -march=genx64 -mcpu=Gen9 -print-before=GenXVisaRegAllocWrapper \
; RUN:   -o /dev/null %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%str = type { i32, i32 }

declare <2 x i32> @llvm.genx.gather.masked.scaled2.v2i32.v2i32.v2i1(i32, i16, i32, i32, <2 x i32>, <2 x i1>)

; CHECK-LABEL: define <2 x i32> @test(
; CHECK: i32 [v][[A0:%.*]], i32 [v][[A1:%.*]])
; CHECK-NEXT: [v,-][{{[0-9]+}}] [[S0:%.*]] = insertvalue %str undef, i32 [[A0]], 0
; CHECK-NEXT: [v,v][{{[0-9]+}}] [[S1:%.*]] = insertvalue %str [[S0]], i32 [[A1]], 1
; CHECK-NEXT: [v][{{[0-9]+}}] [[RET:%.*]] = call spir_func <2 x i32> @proc(%str [[S1]])
; CHECK-NEXT: [{{[0-9]+}}] ret <2 x i32> [[RET]]
define spir_func <2 x i32> @test(i32 %a0, i32 %a1) #0 !FuncArgSize !1 !FuncRetSize !2 {
  %s0 = insertvalue %str undef, i32 %a0, 0
  %s1 = insertvalue %str %s0, i32 %a1, 1
  %ret = call spir_func <2 x i32> @proc(%str %s1)
  ret <2 x i32> %ret
}

; CHECK-LABEL: define <2 x i32> @proc(
; CHECK: [v,v][[ARG:%.*]])
; CHECK-NEXT: [v][{{[0-9]+}}]  [[T0:%.*]] = extractvalue %str [[ARG]], 0
; CHECK-NEXT: [v][{{[0-9]+}}]  [[T1:%.*]] = extractvalue %str [[ARG]], 1
; CHECK-NEXT: [v][{{[0-9]+}}]  [[V02:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.i32.i16.i1(<2 x i32> undef, i32 [[T0]], i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: [v][{{[0-9]+}}]  [[V11:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.i32.i16.i1(<2 x i32> [[V02]], i32 [[T1]], i32 0, i32 1, i32 1, i16 4, i32 0, i1 true)
; CHECK-NEXT: [t][{{[0-9]+}}]  [[_CATEGORYCONV:%.*]] = call i32 @llvm.genx.convert.i32(i32 1)
; CHECK-NEXT: [p][{{[0-9]+}}]  [[P:%.*]] = icmp eq <2 x i32> [[V11]], zeroinitializer
; CHECK-NEXT: [v][{{[0-9]+}}]  [[RET:%.*]] = call <2 x i32> @llvm.genx.gather.masked.scaled2.v2i32.v2i32.v2i1(i32 0, i16 0, i32 [[_CATEGORYCONV]], i32 0, <2 x i32> [[V11]], <2 x i1> [[P]])
; CHECK-NEXT: [{{[0-9]+}}]  ret <2 x i32> [[RET]]
define internal spir_func <2 x i32> @proc(%str %arg) #1 {
  %t0 = extractvalue %str %arg, 0
  %t1 = extractvalue %str %arg, 1
  %v0 = insertelement <2 x i32> undef, i32 %t0, i32 0
  %v1 = insertelement <2 x i32> %v0, i32 %t1, i32 1
  %p = icmp eq <2 x i32> %v1, zeroinitializer
  %ret = call <2 x i32> @llvm.genx.gather.masked.scaled2.v2i32.v2i32.v2i1(i32 0, i16 0, i32 1, i32 0, <2 x i32> %v1, <2 x i1> %p)
  ret <2 x i32> %ret
}

attributes #0 = { "CMStackCall" }
attributes #1 = { noinline }

!1 = !{i32 2}
!2 = !{i32 2}
