;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i8> @some.intrinsic()

define internal spir_func void @test1() {

; CHECK: %y1.shuffle.rd = call <4 x i8> @llvm.genx.rdregioni.v4i8.v32i8.i16(<32 x i8> %x1, i32 0, i32 4, i32 2, i16 2, i32 undef)
; CHECK-NEXT:  %y1.shuffle.wr = call <32 x i8> @llvm.genx.wrregioni.v32i8.v4i8.i16.i1(<32 x i8> undef, <4 x i8> %y1.shuffle.rd, i32 0, i32 4, i32 1, i16 1, i32 undef, i1 true)
  %x1 = call <32 x i8> @some.intrinsic()
  %y1 = shufflevector <32 x i8> %x1, <32 x i8> undef, <32 x i32> <i32 undef, i32 2, i32 4, i32 undef, i32 8, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

; CHECK: %y2.shuffle.rd = call <10 x i8> @llvm.genx.rdregioni.v10i8.v32i8.i16(<32 x i8> %x2, i32 0, i32 5, i32 1, i16 5, i32 undef)
  %x2 = call <32 x i8> @some.intrinsic()
  %y2 = shufflevector <32 x i8> %x2, <32 x i8> undef, <32 x i32> <i32 undef, i32 undef, i32 5, i32 undef, i32 undef, i32 undef, i32 9, i32 5, i32 undef, i32 7, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

  ; CHECK: %y3.shuffle.rd = call <12 x i8> @llvm.genx.rdregioni.v12i8.v32i8.i16(<32 x i8> %x3, i32 1, i32 4, i32 0, i16 0, i32 undef)
  %x3 = call <32 x i8> @some.intrinsic()
  %y3 = shufflevector <32 x i8> %x3, <32 x i8> undef, <32 x i32> <i32 0, i32 0, i32 0, i32 undef, i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 2, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

; CHECK: %y4.shuffle.rd = call <6 x i8> @llvm.genx.rdregioni.v6i8.v32i8.i16(<32 x i8> %x4, i32 3, i32 3, i32 10, i16 2, i32 undef)
  %x4 = call <32 x i8> @some.intrinsic()
  %y4 = shufflevector <32 x i8> %x4, <32 x i8> undef, <32 x i32> <i32 undef, i32 2, i32 12, i32 undef, i32 5, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

; CHECK: %y5.shuffle.rd = call <3 x i8> @llvm.genx.rdregioni.v3i8.v32i8.i16(<32 x i8> %x5, i32 0, i32 3, i32 1, i16 29, i32 undef)
; CHECK-NEXT: %y5.shuffle.rd{{[0-9]+}} = call <3 x i8> @llvm.genx.rdregioni.v3i8.v32i8.i16(<32 x i8> %x5, i32 0, i32 3, i32 1, i16 1, i32 undef)
; CHECK-NEXT: %y5.shuffle.rd{{[0-9]+}} = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x5, i32 0, i32 1, i32 {{[0-9]+}}, i16 30, i32 undef)
  %x5 = call <32 x i8> @some.intrinsic()
  %y5 = shufflevector <32 x i8> %x5, <32 x i8> undef, <32 x i32> <i32 undef, i32 29, i32 30, i32 undef, i32 undef, i32 undef, i32 undef, i32 1, i32 2, i32 3, i32 30, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

; CHECK: %y6.shuffle.rd = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> undef, i32 0, i32 1, i32 {{[0-9]+}}, i16 30, i32 undef)
; CHECK-NEXT: %y6.shuffle.rd{{[0-9]+}} = call <3 x i8> @llvm.genx.rdregioni.v3i8.v32i8.i16(<32 x i8> %x6, i32 0, i32 3, i32 3, i16 0, i32 undef)
  %x6 = call <32 x i8> @some.intrinsic()
  %y6 = shufflevector <32 x i8> undef, <32 x i8> %x6, <32 x i32> <i32 30, i32 undef, i32 32, i32 undef, i32 38, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

  ; CHECK: %y7.shuffle.rd = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x7_1, i32 0, i32 1, i32 {{[0-9]+}}, i16 30, i32 undef)
; CHECK-NEXT: %y7.shuffle.rd{{[0-9]+}} = call <3 x i8> @llvm.genx.rdregioni.v3i8.v32i8.i16(<32 x i8> %x7_2, i32 0, i32 3, i32 3, i16 0, i32 undef)
  %x7_1 = call <32 x i8> @some.intrinsic()
  %x7_2 = call <32 x i8> @some.intrinsic()
  %y7 = shufflevector <32 x i8> %x7_1, <32 x i8> %x7_2, <32 x i32> <i32 30, i32 undef, i32 32, i32 undef, i32 38, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

  ; CHECK:  %y8.shuffle.rd = call <6 x i8> @llvm.genx.rdregioni.v6i8.v32i8.i16(<32 x i8> %x8, i32 0, i32 3, i32 1, i16 29, i32 undef)
  %x8 = call <32 x i8> @some.intrinsic()
  %y8 = shufflevector <32 x i8> %x8, <32 x i8> undef, <32 x i32> <i32 29, i32 30, i32 undef, i32 undef, i32 30, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>

  ; CHECK: %y9.shuffle.rd = call <2 x i8> @llvm.genx.rdregioni.v2i8.v32i8.i16(<32 x i8> %x9_1, i32 0, i32 2, i32 1, i16 29, i32 undef)
  ; CHECK: %y9.shuffle.rd1 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x9_2, i32 0, i32 1, i32 1, i16 0, i32 undef)
  ; CHECK: %y9.shuffle.rd2 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x9_1, i32 0, i32 1, i32 1, i16 30, i32 undef)
  ; CHECK: %y9.shuffle.rd3 = call <3 x i8> @llvm.genx.rdregioni.v3i8.v32i8.i16(<32 x i8> %x9_2, i32 0, i32 3, i32 1, i16 2, i32 undef)
  ; CHECK: %y9.shuffle.rd4 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x9_1, i32 0, i32 1, i32 1, i16 20, i32 undef)
  ; CHECK: %y9.shuffle.rd5 = call <1 x i8> @llvm.genx.rdregioni.v1i8.v32i8.i16(<32 x i8> %x9_2, i32 0, i32 1, i32 1, i16 6, i32 undef)
  %x9_1 = call <32 x i8> @some.intrinsic()
  %x9_2 = call <32 x i8> @some.intrinsic()
  %y9 = shufflevector <32 x i8> %x9_1, <32 x i8> %x9_2, <32 x i32> <i32 29, i32 30, i32 undef, i32 32, i32 30, i32 undef, i32 undef, i32 34, i32 35, i32 36, i32 20, i32 38, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>


  ret void
}

