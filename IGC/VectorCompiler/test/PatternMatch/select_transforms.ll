;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define <16 x i32> @test1(<16 x i32> %val1, <16 x i32> %val2, <16 x i1> %cond) {
; CHECK-LABEL: test1
; CHECK: %1 = or <16 x i32> %val1, %val2
; CHECK-NEXT: %2 = select <16 x i1> %cond, <16 x i32> %1, <16 x i32> %val2
  %1 = select <16 x i1> %cond, <16 x i32> %val1, <16 x i32> zeroinitializer
  %2 = or <16 x i32> %1, %val2
  ret <16 x i32> %2
}

declare <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16(<16 x i16>, i32, i32, i32, i16, i32)
declare <16 x i16> @llvm.genx.wrregioni.v16i16(<16 x i16>, <4 x i16>, i32, i32, i32, i16, i32, i1)

define <16 x i16> @test2(<16 x i16> %rgn, <4 x i16> %val, <4 x i1> %cond) {
; CHECK-LABEL: test2
; CHECK:  %cond.inv = xor <4 x i1> %cond, <i1 true, i1 true, i1 true, i1 true>
; CHECK-NEXT: %1 = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16(<16 x i16> %rgn, i32 0, i32 4, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %2 = call <16 x i16> @llvm.genx.wrregioni.v16i16.v4i16.i16.v4i1(<16 x i16> %rgn, <4 x i16> %val, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1> %cond.inv)
  %1 = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16(<16 x i16> %rgn, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = select <4 x i1> %cond, <4 x i16> %1, <4 x i16> %val
  %3 = call <16 x i16> @llvm.genx.wrregioni.v16i16(<16 x i16> %rgn, <4 x i16> %2, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 1)
  ret <16 x i16> %3
}

define <16 x i16> @test3(<16 x i16> %rgn, <4 x i16> %val, <4 x i1> %cond) {
; CHECK-LABEL: test3
; CHECK:  %cond.inv = xor <4 x i1> %cond, <i1 true, i1 true, i1 true, i1 true>
; CHECK-NEXT: %1 = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16(<16 x i16> %rgn, i32 0, i32 4, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %2 = call <16 x i16> @llvm.genx.wrregioni.v16i16.v4i16.i16.v4i1(<16 x i16> %rgn, <4 x i16> %val, i32 0, i32 4, i32 1, i16 0, i32 undef, <4 x i1> %cond.inv)
  %1 = call <4 x i16> @llvm.genx.rdregioni.v4i16.v16i16(<16 x i16> %rgn, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = select <4 x i1> %cond, <4 x i16> %1, <4 x i16> %val
  %3 = call <16 x i16> @llvm.genx.wrregioni.v16i16(<16 x i16> undef, <4 x i16> %2, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 1)
  ret <16 x i16> %3
}

declare <4 x i16> @llvm.genx.rdregioni.v4i16.v32i16(<32 x i16>, i32, i32, i32, i16, i32)

define void @test4(<32 x i16> %rgn, <4 x i16> %val, <4 x i1> %cond) {
; CHECK-LABEL: test4
; CHECK: select
  %1 = call <4 x i16> @llvm.genx.rdregioni.v4i16.v32i16(<32 x i16> %rgn, i32 0, i32 4, i32 1, i16 0, i32 undef)
  %2 = select <4 x i1> %cond, <4 x i16> %1, <4 x i16> %val
  %3 = call <16 x i16> @llvm.genx.wrregioni.v16i16(<16 x i16> undef, <4 x i16> %2, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 1)
  ret void
}

