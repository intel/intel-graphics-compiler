;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1) readnone nounwind
declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32>, i32, i32, i32, i16, i32) readnone nounwind

define <8 x i32> @test1(<16 x i32> %arg1, <8 x i32> %arg2) {
; CHECK-NOT: %bypass = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %arg1, <8 x i32> %arg2, i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  %bypass = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %arg1, <8 x i32> %arg2, i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK: %rdregion = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32> %arg1, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rdregion = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16.i1(<16 x i32> %bypass, i32 0, i32 8, i32 1, i16 0, i32 undef)
  ret <8 x i32> %rdregion
}

declare <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1>, <8 x i1>, i32) readnone nounwind
declare <8 x i1> @llvm.genx.rdpredregion.v8i1.v16i1(<16 x i1>, i32) readnone nounwind

define <8 x i1> @test2(<16 x i1> %arg1, <8 x i1> %arg2) {
; CHECK-NOT: %bypass = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> %arg1, <8 x i1> %arg2, i32 8)
  %bypass = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> %arg1, <8 x i1> %arg2, i32 8)
; CHECK: %rdpredregion = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v16i1(<16 x i1> %arg1, i32 0)
  %rdpredregion = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v16i1(<16 x i1> %bypass, i32 0)
  ret <8 x i1> %rdpredregion
}
