;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i4> @some.intrinsic()

define internal spir_func void @test1() {
  %x = call <32 x i4> @some.intrinsic()
; CHECK:  %y.shuffle.rd = call <32 x i4> @llvm.genx.rdregioni.v32i4.v32i4.i16(<32 x i4> %x, i32 1, i32 2, i32 0, i16 0, i32 undef)
  %y = shufflevector <32 x i4> %x, <32 x i4> undef, <32 x i32> <i32 0, i32 0, i32 1, i32 1, i32 2, i32 2, i32 3, i32 3, i32 4, i32 4, i32 5, i32 5, i32 6, i32 6, i32 7, i32
7, i32 8, i32 8, i32 9, i32 9, i32 10, i32 10, i32 11, i32 11, i32 12, i32 12, i32 13, i32 13, i32 14, i32 14, i32 15, i32 15>
  ret void
}

