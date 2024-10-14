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

; CHECK-LABEL: @test_rip_dependency
define <32 x i32> @test_rip_dependency(<32 x i32> %dst, <16 x i32> %a, <16 x i32> %b, <16 x i32> %c) {
; CHECK: %ins.b = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %b, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK: %ins.c = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.b, <16 x i32> %c, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %ins.a = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %dst, <16 x i32> %a, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %ins.b = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.a, <16 x i32> %b, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ins.c = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %ins.b, <16 x i32> %c, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ret <32 x i32> %ins.c
}

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
