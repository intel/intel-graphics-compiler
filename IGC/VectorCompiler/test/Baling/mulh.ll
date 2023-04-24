;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <2 x i32> @llvm.genx.smulh.v2i32.v2i32(<2 x i32>, <2 x i32>)

; CHECK: GenXBaling dump start
; CHECK: bales in function: test:
; CHECK-NOT: llvm.genx.smulh.v2i32{{[^)]*}}): maininst
; CHECK: GenXBaling dump end
define <2 x i32> @test(<2 x i8> %val) {
  %1 = sext <2 x i8> %val to <2 x i32>
  %2 = call <2 x i32> @llvm.genx.smulh.v2i32.v2i32(<2 x i32> %1, <2 x i32> <i32 3, i32 3>)
  ret <2 x i32> %2
}
