;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i1> @some.intrinsic.32i1()
declare <4 x i1> @some.intrinsic.4i1()
define internal spir_func void @test1() {
  %x = call <32 x i1> @some.intrinsic.32i1()
; CHECK:  %y{{[0-9]+}}.shuffle.rd = call <32 x i16> @llvm.genx.rdregioni.v32i16.v32i16.i16(<32 x i16> %{{[0-9]+}}, i32 1, i32 2, i32 0, i16 0, i32 undef)
  %y = shufflevector <32 x i1> %x, <32 x i1> undef, <32 x i32> <i32 0, i32 0, i32 1, i32 1, i32 2, i32 2, i32 3, i32 3, i32 4, i32 4, i32 5, i32 5, i32 6, i32 6, i32 7, i32
7, i32 8, i32 8, i32 9, i32 9, i32 10, i32 10, i32 11, i32 11, i32 12, i32 12, i32 13, i32 13, i32 14, i32 14, i32 15, i32 15>
; CHECK: %z{{[0-9]+}}.shuffle.rd
; CHECK-NOT: shufflevector <32 x i{{[0-9]+}}>
  %z = shufflevector <32 x i1> %y, <32 x i1> undef, <32 x i32> <i32 6, i32 5, i32 7, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32
undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

  %narrow = call <4 x i1> @some.intrinsic.4i1()
  %wide = shufflevector <4 x i1> %narrow, <4 x i1> undef, <32 x i32> <i32 3, i32 1, i32 0, i32 undef, i32 2, i32 2, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 0, i32 undef, i32 undef, i32 undef, i32
undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

ret void
}

; CHECK-LABEL: test2
; CHECK:  %val.splat = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> %val, i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK:  %.splat = icmp sgt <8 x i32> %val.splat, <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>
; CHECK:  ret <8 x i1> %.splat
define <8 x i1> @test2(<1 x i32> %val) {
  %1 = icmp sgt <1 x i32> %val, <i32 -1>
  %2 = shufflevector <1 x i1> %1, <1 x i1> undef, <8 x i32> zeroinitializer
  ret <8 x i1> %2
}
