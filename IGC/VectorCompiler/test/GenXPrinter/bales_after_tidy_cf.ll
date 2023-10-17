;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Check that bales are printed after tidy control flow.

; RUN: llc -march=genx64 -mcpu=Gen9 -print-after=GenXTidyControlFlow \
; RUN:  -finalizer-opts='-dumpcommonisa' -o /dev/null %s 2>&1 | FileCheck %s

; RUN: FileCheck %s --input-file=_test_f0.visaasm --check-prefix=CHECK_ARGSIZE

; CHECK_ARGSIZE: .kernel_attr ArgSize=2

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; CHECK: @test(i32 [v][[A0:%.*]], <2 x i32> [v][[V1:%.*]])
; [-][{{[0-9]+}}]  [[REG11:%.*]] = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32 8, <96 x i32> undef)
; [-][{{[0-9]+}}]  [[REG10:%.*]] = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32 8, <96 x i32> undef)
; [v][{{[0-9]+}}]  [[RDREGIONI:%.*]] = call i32 @llvm.genx.rdregioni.i32.v96i32.i16(<96 x i32> [[REG11]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; [v][{{[0-9]+}}]  [[V01:%.*]] = bitcast i32 [[RDREGIONI]] to <1 x i32>
; [v][{{[0-9]+}}]  bale {
; [{{[0-9]+}}]     [[V0:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v1i32.i16(<1 x i32> [[V01]], i32 0, i32 2, i32 0, i16 0, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[RDREGIONI9:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v96i32.i16(<96 x i32> [[REG11]], i32 0, i32 2, i32 1, i16 32, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[RES:%.*]] = add <2 x i32> [[V0]], [[RDREGIONI9]]
; [{{[0-9]+}}]  }
; [-][{{[0-9]+}}]  [[REG7:%.*]] = call <64 x i32> @llvm.genx.read.predef.reg.v64i32.v64i32(i32 9, <64 x i32> undef)
; [v][{{[0-9]+}}]  bale {
; [{{[0-9]+}}]     [[V06:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v1i32.i16(<1 x i32> [[V01]], i32 0, i32 2, i32 0, i16 0, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[RDREGIONI8:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v96i32.i16(<96 x i32> [[REG10]], i32 0, i32 2, i32 1, i16 32, i32 undef) {rdregion}
; [{{[0-9]+}}]     [[RES5:%.*]] = add <2 x i32> [[V06]], [[RDREGIONI8]]
; [{{[0-9]+}}]     [[WRREGIONI:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v2i32.i16.i1(<64 x i32> [[REG7]], <2 x i32> [[RES5]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true) {wrregion}
; [{{[0-9]+}}]  }
; [-][{{[0-9]+}}]  [[REG:%.*]] = call <2 x i32> @llvm.genx.write.predef.reg.v2i32.v64i32(i32 9, <64 x i32> [[WRREGIONI]])
; [{{[0-9]+}}]  ret <2 x i32> [[RES]]
define spir_func <2 x i32> @test(i32 %a0, <2 x i32> %v1) #0 !FuncArgSize !1 !FuncRetSize !2 {
  %t0 = insertelement <2 x i32> undef, i32 %a0, i32 0
  %v0 = shufflevector <2 x i32> %t0, <2 x i32> undef, <2 x i32> <i32 0, i32 0>
  %res = add <2 x i32> %v0, %v1
  ret <2 x i32> %res
}

attributes #0 = { "CMStackCall" }

!1 = !{i32 3}
!2 = !{i32 2}
