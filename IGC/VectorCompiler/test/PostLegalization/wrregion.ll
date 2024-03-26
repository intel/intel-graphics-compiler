;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s --check-prefix=XeHPC
; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64 -S < %s | FileCheck %s --check-prefix=XeHPG

declare <48 x i32> @llvm.genx.wrregioni.v48i32.v32i32.i16.i1(<48 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)
declare <32 x i16> @llvm.genx.rdregioni.v32i16.v96i16.i16(<96 x i16>, i32, i32, i32, i16, i32)

; XeHPC-LABEL: test
; XeHPG-LABEL: test
define <32 x i16> @test(<32 x i32> %arg) {
; XeHPC-NEXT: call <48 x i32> @llvm.genx.wrregioni.v48i32.v32i32.i16.i1(<48 x i32> zeroinitializer, <32 x i32> %arg, i32 0, i32 32, i32 1, i16 64, i32 undef, i1 true)
; XeHPG-NEXT: call <40 x i32> @llvm.genx.wrregioni.v40i32.v32i32.i16.i1(<40 x i32> zeroinitializer, <32 x i32> %arg, i32 0, i32 32, i32 1, i16 32, i32 undef, i1 true)
  %1 = call <48 x i32> @llvm.genx.wrregioni.v48i32.v32i32.i16.i1(<48 x i32> zeroinitializer, <32 x i32> %arg, i32 0, i32 32, i32 1, i16 64, i32 undef, i1 true)
  %cast = bitcast <48 x i32> %1 to <192 x i8>
  %postcast = bitcast <192 x i8> %cast to <96 x i16>
  %split = call <32 x i16> @llvm.genx.rdregioni.v32i16.v96i16.i16(<96 x i16> %postcast, i32 32, i32 32, i32 1, i16 62, i32 undef)
  ret <32 x i16> %split
}
