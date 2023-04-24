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

; CHECK_ARGSIZE: .kernel_attr ArgSize=3

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; CHECK: @test(i32 [v][[A0:%.*]], <2 x i32> [v][[V1:%.*]])
; CHECK-NEXT: [[BC:%.*]] = bitcast i32 [[A0]] to <1 x i32>
; CHECK-NEXT: bale {
; CHECK-NEXT:    [[RDR:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v1i32.i16(<1 x i32> [[BC]], i32 0, i32 2, i32 0, i16 0, i32 undef) {rdregion}
; CHECK-NEXT:    [[RES:%.*]] = add <2 x i32> [[RDR]], [[V1]]
; CHECK-NEXT: }
; CHECK-NEXT: ret <2 x i32> [[RES]]
define spir_func <2 x i32> @test(i32 %a0, <2 x i32> %v1) #0 !FuncArgSize !1 !FuncRetSize !2 {
  %t0 = insertelement <2 x i32> undef, i32 %a0, i32 0
  %v0 = shufflevector <2 x i32> %t0, <2 x i32> undef, <2 x i32> <i32 0, i32 0>
  %res = add <2 x i32> %v0, %v1
  ret <2 x i32> %res
}

attributes #0 = { "CMStackCall" }

!1 = !{i32 3}
!2 = !{i32 2}
