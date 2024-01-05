;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <1 x i32> @llvm.genx.lsc.load.slm.v1i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32) nounwind readonly
declare void @llvm.genx.lsc.fence.v32i1(<32 x i1>) nounwind
declare <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) nounwind readnone

; CHECK-LABEL: test
define <16 x i32> @test(<16 x i32> %val) {
  %1 = tail call <1 x i32> @llvm.genx.lsc.load.slm.v1i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, i32 0, i32 0)
  tail call void @llvm.genx.lsc.fence.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK-NOT: %2 = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32> %val, <1 x i32> %1, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true): wrregion 1
  %2 = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32> %val, <1 x i32> %1, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ret <16 x i32> %2
}
