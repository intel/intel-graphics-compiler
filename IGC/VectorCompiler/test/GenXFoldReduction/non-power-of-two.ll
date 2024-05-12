;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXFoldReduction -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; REQUIRES: llvm_12_or_greater

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"

declare <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32)
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v6i32.i16(<6 x i32>, i32, i32, i32, i16, i32)
declare <3 x i32> @llvm.genx.rdregioni.v3i32.v7i32.i16(<7 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v5i32.i16(<5 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v6i32.i16(<6 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32>, i32, i32, i32, i16, i32)

; COM: The algorithm doesn't match reduction of 2 and 3 element vectors
; CHECK-LABEL: @test_v2(
define i32 @test_v2(<2 x i32> %src) {
  ; CHECK: %reduce = add i32
  %1 = extractelement <2 x i32> %src, i32 0
  %2 = extractelement <2 x i32> %src, i32 1
  %reduce = add i32 %1, %2
  ret i32 %reduce
}

; CHECK-LABEL: @test_v3
define i32 @test_v3(<3 x i32> %src) {
  ; CHECK: [[STEP:%[^ ]+]] = add i32
  ; CHECK: %reduce = add i32 [[STEP]],
  %1 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %src, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %2 = extractelement <3 x i32> %src, i32 2
  %3 = extractelement <2 x i32> %1, i32 0
  %4 = extractelement <2 x i32> %1, i32 1
  %5 = add i32 %3, %4
  %reduce = add i32 %5, %2
  ret i32 %reduce
}

; CHECK-LABEL: @test_v5
define i32 @test_v5(<5 x i32> %src) {
  ; CHECK: %reduce = call i32 @llvm.vector.reduce.add.v5i32(<5 x i32> %src)
  %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v5i32.i16(<5 x i32> %src, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = extractelement <5 x i32> %src, i32 4
  %3 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %4 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %5 = add <2 x i32> %3, %4
  %6 = extractelement <2 x i32> %5, i32 0
  %7 = extractelement <2 x i32> %5, i32 1
  %8 = add i32 %6, %7
  %reduce = add i32 %8, %2
  ret i32 %reduce
}

; CHECK-LABEL: @test_v6
define i32 @test_v6(<6 x i32> %src) {
  ; CHECK: %reduce = call i32 @llvm.vector.reduce.add.v6i32(<6 x i32> %src)
  %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v6i32.i16(<6 x i32> %src, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v6i32.i16(<6 x i32> %src, i32 0, i32 2, i32 1, i16 16, i32 undef)
  %3 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %4 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %5 = add <2 x i32> %3, %4
  %6 = add <2 x i32> %5, %2
  %7 = extractelement <2 x i32> %6, i32 0
  %8 = extractelement <2 x i32> %6, i32 1
  %reduce = add i32 %7, %8
  ret i32 %reduce
}

; CHECK-LABEL: @test_v7
define i32 @test_v7(<7 x i32> %src) {
  ; CHECK: %reduce = call i32 @llvm.vector.reduce.add.v7i32(<7 x i32> %src)
  %1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %src, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = call <3 x i32> @llvm.genx.rdregioni.v3i32.v7i32.i16(<7 x i32> %src, i32 0, i32 3, i32 1, i16 16, i32 undef)
  %3 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %4 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %1, i32 0, i32 2, i32 1, i16 8, i32 undef)
  %5 = add <2 x i32> %3, %4
  %6 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %2, i32 0, i32 2, i32 1, i16 0, i32 undef)
  %7 = extractelement <3 x i32> %2, i32 2
  %8 = add <2 x i32> %5, %6
  %9 = extractelement <2 x i32> %8, i32 0
  %10 = extractelement <2 x i32> %8, i32 1
  %11 = add i32 %9, %10
  %reduce = add i32 %11, %7
  ret i32 %reduce
}
