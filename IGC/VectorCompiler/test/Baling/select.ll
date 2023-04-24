;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define <16 x i16> @test(<16 x i16> %oldval, <16 x i16> %val1, <16 x i16> %val2, <16 x i1> %cond) {
; CHECK:  %1 = or <16 x i16> %val1, %val2
; CHECK-NEXT:  %2 = call <16 x i16> @llvm.genx.wrregioni.v16i16.v16i16.i16.v16i1(<16 x i16> %oldval, <16 x i16> %1, i32 0, i32 16, i32 1, i16 0, i32 undef, <16 x i1> %cond)
  %1 = or <16 x i16> %val1, %val2
  %2 = select <16 x i1> %cond, <16 x i16> %1, <16 x i16> %oldval
  ret <16 x i16> %2
}
