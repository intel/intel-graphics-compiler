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
; [-][{{[0-9]+}}]  [[REG6:%.*]] = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32 8, <96 x i32> undef)
; [v][{{[0-9]+}}]  bale {
; [{{[0-9]+}}]     [[RDREGIONI5:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v96i32.i16(<96 x i32> [[REG6]], i32 0, i32 1, i32 1, i16 0, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[WRREGIONI:%.*]] = call <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.i16.i1(<1 x i32> undef, <1 x i32> [[RDREGIONI5]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true) {wrregion}
; [{{[0-9]+}}]  }
; [v][{{[0-9]+}}]  [[BITCAST:%.*]] = bitcast <1 x i32> [[WRREGIONI]] to i32
; [v][{{[0-9]+}}]  bale {
; [{{[0-9]+}}]     [[RDREGIONI:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v96i32.i16(<96 x i32> [[REG6]], i32 0, i32 1, i32 1, i16 32, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[WRREGIONI1:%.*]] = call <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.i16.i1(<1 x i32> undef, <1 x i32> [[RDREGIONI]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true) {wrregion}
; [{{[0-9]+}}]  }
; [v][{{[0-9]+}}]  [[BITCAST2:%.*]] = bitcast <1 x i32> [[WRREGIONI1]] to i32
; [v,-][{{[0-9]+}}]  [[S0:%.*]] = insertvalue %str undef, i32 [[BITCAST]], 0
; [v,v][{{[0-9]+}}]  [[S1:%.*]] = insertvalue %str [[S0]], i32 [[BITCAST2]], 1
; [v][{{[0-9]+}}]  [[RET:%.*]] = call spir_func <2 x i32> @proc(%str [[S1]])
; [-][{{[0-9]+}}]  [[REG4:%.*]] = call <64 x i32> @llvm.genx.read.predef.reg.v64i32.v64i32(i32 9, <64 x i32> undef)
; [v][{{[0-9]+}}]  [[WRREGIONI3:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v2i32.i16.i1(<64 x i32> [[REG4]], <2 x i32> [[RET]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
; [-][{{[0-9]+}}]  [[REG:%.*]] = call <2 x i32> @llvm.genx.write.predef.reg.v2i32.v64i32(i32 9, <64 x i32> [[WRREGIONI3]])
; [{{[0-9]+}}]  ret <2 x i32> [[RET]]
define spir_func <2 x i32> @test(i32 %a0, i32 %a1) #0 !FuncArgSize !1 !FuncRetSize !2 {
  %s0 = insertvalue %str undef, i32 %a0, 0
  %s1 = insertvalue %str %s0, i32 %a1, 1
  %ret = call spir_func <2 x i32> @proc(%str %s1)
  ret <2 x i32> %ret
}

; CHECK-LABEL: define <2 x i32> @proc(
; CHECK: [v,v][[ARG:%.*]])
; [v][{{[0-9]+}}]  [[T0:%.*]] = extractvalue %str %arg, 0
; [v][{{[0-9]+}}]  [[T1:%.*]] = extractvalue %str %arg, 1
; [v][{{[0-9]+}}]  [[V02:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.i32.i16.i1(<2 x i32> undef, i32 [[T0]], i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
; [v][{{[0-9]+}}]  [[V11:%.*]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.i32.i16.i1(<2 x i32> [[V02]], i32 [[T1]], i32 0, i32 1, i32 1, i16 4, i32 0, i1 true)
; [t][{{[0-9]+}}]  [[CATEGORYCONV:%.*]] = call i32 @llvm.genx.convert.i32(i32 1)
; [p][{{[0-9]+}}]  [[P:%.*]] = icmp eq <2 x i32> [[V11]], zeroinitializer
; [v][{{[0-9]+}}]  [[RET:%.*]] = call <2 x i32> @llvm.genx.gather.masked.scaled2.v2i32.v2i32.v2i1(i32 0, i16 0, i32 [[CATEGORYCONV]], i32 0, <2 x i32> %v11, <2 x i1> [[P]])
; [{{[0-9]+}}]  ret <2 x i32> [[RET]]
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
