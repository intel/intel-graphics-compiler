;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i1> @some.intrinsic1()

; CHECK-LABEL: test1
define internal spir_func void @test1() {
  %cmpres = call <32 x i1> @some.intrinsic1()

  ; CHECK: llvm.genx.rdregioni.v32i16.v32i16.i16(<32 x i16> %{{[0-9]+}}, i32 0, i32 32, i32 0, i16 4, i32 undef)
  %shift = shufflevector <32 x i1> %cmpres, <32 x i1> undef, <32 x i32> <i32 undef, i32 2, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32    undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  ret void
}

declare i64 @some.intrinsic2()

; CHECK-LABEL: test2
define internal spir_func void @test2() {
  %x = call i64 @some.intrinsic2()

  ; CHECK: llvm.genx.wrregioni.v16i64.i64.i16.i1(<16 x i64> undef, i64 %x, i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
  %splat.splatinsert.i.i = insertelement <16 x i64> undef, i64 %x, i32 0

  ; CHECK: llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64> %{{[A-Za-z0-9.]+}}, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %splat.splat.i.i = shufflevector <16 x i64> %splat.splatinsert.i.i, <16 x i64> undef, <16 x i32> zeroinitializer
  ret void
}

declare <32 x float> @some.intrinsic3()

; CHECK-LABEL: test3
define internal spir_func void @test3() {
  %x = call <32 x float> @some.intrinsic3()

  ; CHECK: llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %x, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %splat.splat.i303.i = shufflevector <32 x float> %x, <32 x float> undef, <16 x i32> zeroinitializer
  ret void
}

declare <32 x float> @some.intrinsic4()

; CHECK-LABEL: test4
define internal spir_func void @test4() {
  %x = call <32 x float> @some.intrinsic4()

  ; CHECK: llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> %x, i32 0, i32 16, i32 0, i16 4, i32 undef)
  %splat.splat.i421.i = shufflevector <32 x float> %x, <32 x float> undef, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret void
}

