;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x float> @llvm.genx.ieee.sqrt.v16f32(<16 x float>)
declare <16 x float> @llvm.genx.ieee.div.v16f32(<16 x float>, <16 x float>)

; NO SPLIT
; CHECK-LABEL: @test1
; CHECK: add <16 x i32>
define <16 x i32> @test1(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test2
; CHECK: add <16 x i32>
define <16 x i32> @test2(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test3
; CHECK: add <16 x i32>
define <16 x i32> @test3(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}

; SPLIT 8
; CHECK-LABEL: @test4
; CHECK: add <8 x i32>
; CHECK: add <8 x i32>
define <16 x i32> @test4(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test5
; CHECK: add <8 x i32>
; CHECK: add <8 x i32>
define <16 x i32> @test5(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test6
; CHECK: add <4 x i32>
; CHECK: add <8 x i32>
; CHECK: add <4 x i32>
define <16 x i32> @test6(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}

; SPLIT 4
; CHECK-LABEL: @test7
; CHECK: add <4 x i32>
; CHECK: add <8 x i32>
; CHECK: add <4 x i32>
define <16 x i32> @test7(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 1, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test8
; CHECK: add <8 x i32>
; CHECK: add <4 x i32>
; CHECK: add <4 x i32>
define <16 x i32> @test8(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 1>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test9
; CHECK: add <4 x i32>
; CHECK: add <4 x i32>
; CHECK: add <8 x i32>
define <16 x i32> @test9(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}

; SPLIT 2
; CHECK-LABEL: @test10
; CHECK: add <2 x i32>
; CHECK: add <8 x i32>
; CHECK: add <4 x i32>
; CHECK: add <2 x i32>
define <16 x i32> @test10(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 1, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test11
; CHECK: add <8 x i32>
; CHECK: add <2 x i32>
; CHECK: add <4 x i32>
; CHECK: add <2 x i32>
define <16 x i32> @test11(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test12
; CHECK: add <4 x i32>
; CHECK: add <2 x i32>
; CHECK: add <8 x i32>
; CHECK: add <2 x i32>
define <16 x i32> @test12(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}

; SPLIT 1
; CHECK-LABEL: @test13
; CHECK: add <1 x i32>
; CHECK: add <8 x i32>
; CHECK: add <4 x i32>
; CHECK: add <2 x i32>
; CHECK: add <1 x i32>
define <16 x i32> @test13(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}
; CHECK-LABEL: @test14
; CHECK: add <4 x i32>
; CHECK: add <1 x i32>
; CHECK: add <8 x i32>
; CHECK: add <2 x i32>
; CHECK: add <1 x i32>
define <16 x i32> @test14(<16 x i32> %arg) {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %and = and <16 x i32> %add, <i32 0, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <16 x i32> %and
}

; NO SPLIT FOR HALF
; CHECK-LABEL: @test15
; CHECK: fadd <16 x half>
define <16 x i16> @test15(<16 x half> %arg) {
  %add = fadd <16 x half> %arg, <half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0, half 1.0>
  %int = fptoui <16 x half> %add to <16 x i16>
  %and = and <16 x i16> %int, <i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>
  ret <16 x i16> %and
}

; NO SPLIT FOR IEEE INTRINSICS
; CHECK-LABEL: @test16
; CHECK:       call <16 x float> @llvm.genx.ieee.sqrt.v16f32
; CHECK-NEXT:  call <16 x float> @llvm.genx.ieee.div.v16f32
define <16 x i16> @test16(<16 x float> %arg) {
  %sqrt = call <16 x float> @llvm.genx.ieee.sqrt.v16f32(<16 x float> %arg)
  %div = call <16 x float> @llvm.genx.ieee.div.v16f32(<16 x float> %arg, <16 x float> %sqrt)
  %int = fptoui <16 x float> %div to <16 x i16>
  %and = and <16 x i16> %int, <i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>
  ret <16 x i16> %and
}
