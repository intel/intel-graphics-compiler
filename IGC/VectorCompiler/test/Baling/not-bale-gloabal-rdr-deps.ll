;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; %rdr shouldn't bale into %wrregion2 as there is store in between
@global_vec = internal global <8 x i32> undef, align 32 #0
define <8 x i32>  @test() {
  %gload = load volatile <8 x i32>, <8 x i32>* @global_vec
; CHECK: %rdr = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v8i32.i16(<8 x i32> %gload, i32 0, i32 1, i32 0, i16 20, i32 undef): rdregion
  %rdr = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v8i32.i16(<8 x i32> %gload, i32 0, i32 1, i32 0, i16 20, i32 undef)
  %gload3 = load volatile <8 x i32>, <8 x i32>* @global_vec
  %rdr1 = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v8i32.i16(<8 x i32> %gload3, i32 0, i32 1, i32 0, i16 8, i32 undef)
  %gload4 = load volatile <8 x i32>, <8 x i32>* @global_vec
  %wrregion = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %gload4, <1 x i32> %rdr1, i32 0, i32 1, i32 0, i16 20, i32 undef, i1 true)
  store volatile <8 x i32> %wrregion, <8 x i32>* @global_vec
  %gload5 = load volatile <8 x i32>, <8 x i32>* @global_vec
; CHECK-NOT: %wrregion2 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %gload5, <1 x i32> %rdr, i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true): wrregion 1
  %wrregion2 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %gload5, <1 x i32> %rdr, i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true)
  store volatile <8 x i32> %wrregion2, <8 x i32>* @global_vec
  %gload6 = load volatile <8 x i32>, <8 x i32>* @global_vec
  ret <8 x i32> %gload6
}

declare <1 x i32> @llvm.genx.rdregioni.v1i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)

attributes #0 = { "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
