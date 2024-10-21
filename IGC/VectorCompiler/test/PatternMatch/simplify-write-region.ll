;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)
declare <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32>, <32 x i32>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_fold_write_region
define <128 x i32> @test_fold_write_region(<128 x i32> %dst, <32 x i32> %tmp, <16 x i32> %a, <16 x i32> %b, <16 x i32> %c) {
; CHECK: [[INS1:%.+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> %dst, <16 x i32> %b, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true)
; CHECK: [[INS2:%.+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> [[INS1]], <16 x i32> %c, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true)
; CHECK: ret <128 x i32> [[INS2]]
  %ins.a = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %tmp, <16 x i32> %a, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %ins.b = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.a, <16 x i32> %b, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ins.c = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.b, <16 x i32> %c, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %res = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %dst, <32 x i32> %ins.c, i32 0, i32 32, i32 1, i16 128, i32 undef, i1 true)
  ret <128 x i32> %res
}

; CHECK-LABEL: @test_fold_write_region_2
define <128 x i32> @test_fold_write_region_2(<128 x i32> %dst, <16 x i32> %b, <16 x i32> %c) {
; CHECK: [[INS1:%.+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> %dst, <16 x i32> %b, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true)
; CHECK: [[INS2:%.+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> [[INS1]], <16 x i32> %c, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true)
; CHECK: ret <128 x i32> [[INS2]]
  %ins.b = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %b, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ins.c = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.b, <16 x i32> %c, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %res = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %dst, <32 x i32> %ins.c, i32 0, i32 32, i32 1, i16 128, i32 undef, i1 true)
  ret <128 x i32> %res
}

; CHECK-LABEL: @test_fold_write_region_3
define <128 x i32> @test_fold_write_region_3(<128 x i32> %dst, <16 x i32> %c) {
; CHECK: [[INS:%.+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> %dst, <16 x i32> %c, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true)
; CHECK: ret <128 x i32> [[INS]]
  %ins.c = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %c, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %res = call <128 x i32> @llvm.genx.wrregioni.v128i32.v32i32.i16.i1(<128 x i32> %dst, <32 x i32> %ins.c, i32 0, i32 32, i32 1, i16 128, i32 undef, i1 true)
  ret <128 x i32> %res
}

declare <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32, i64)
declare <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64>, <1 x i64>, i32, i32, i32, i16, i32, i1)
declare <8 x i64> @llvm.genx.wrregioni.v8i64.v1i64.i16.i1(<8 x i64>, <1 x i64>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_ignore_predef
define <8 x i64> @test_ignore_predef(<8 x i64> %dst) {
; CHECK: %sp = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %rd = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %sp, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %wr = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %rd, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %res = call <8 x i64> @llvm.genx.wrregioni.v8i64.v1i64.i16.i1(<8 x i64> %dst, <1 x i64> %wr, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: ret <8 x i64> %res
  %sp = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
  %rd = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %sp, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %wr = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %rd, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %res = call <8 x i64> @llvm.genx.wrregioni.v8i64.v1i64.i16.i1(<8 x i64> %dst, <1 x i64> %wr, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ret <8 x i64> %res
}
