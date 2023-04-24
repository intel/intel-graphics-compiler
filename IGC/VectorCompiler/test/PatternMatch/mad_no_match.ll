;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true --fp-contract=fast  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true --fp-contract=off  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s |  FileCheck  --check-prefix=NoFpMad %s


; order wrong
define <32 x i16> @test_nomatch_v32i16(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)  {
  ;CHECK: %1 = add <32 x i16> %op0, %op1
  ;CHECK-NEXT: %2 = mul <32 x i16> %1, %op2

  %1 = add <32 x i16> %op0, %op1
  %2 = mul <32 x i16> %1, %op2

  ret <32 x i16> %2
}

; optimization not occur of -fp-contract=off
define <16 x float> @test_nomatch_v32float(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2)  {
  ;NoFpMad: %1 = fmul <16 x float> %op0, %op1
  ;NoFpMad-NEXT: %2 = fadd <16 x float> %1, %op2
  ;CHECK: [[V1:%[^ ]+]] = call <16 x float> @llvm.fma.v16f32(<16 x float> %op0, <16 x float> %op1, <16 x float> %op2)

  %1 = fmul <16 x float> %op0, %op1
  %2 = fadd <16 x float> %1, %op2

  ret <16 x float> %2
}

declare <32 x i16> @llvm.genx.ssadd.sat.v32i16.v32i16(<32 x i16>, <32 x i16>)
define <32 x i16> @test_ssnomatch_v32i16(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)  {
  ;CHECK: %1 = call <32 x i16> @llvm.genx.ssadd.sat.v32i16.v32i16(<32 x i16> %op0, <32 x i16> %op1)
  ;CHECK-NEXT: %2 = mul <32 x i16> %1, %op2

  %1 = call <32 x i16> @llvm.genx.ssadd.sat.v32i16.v32i16(<32 x i16> %op0, <32 x i16> %op1)
  %2 = mul <32 x i16> %1, %op2

  ret <32 x i16> %2
}

; scalar value negative order different
define i16 @test_match_i16_neg(i16 %op0, i16 %op1, i16 %op2)  {
  ;CHECK: %1 = sub i16 %op0, %op1
  ;CHECK-NEXT: %2 = mul i16 %1, %op2

  %1 = sub i16 %op0, %op1
  %2 = mul i16 %1, %op2

  ret i16 %2
}

; Multiply-Add operation on D-word arguments with SAT cannot be replaced with mad
declare <4 x i32> @llvm.genx.ustrunc.sat.v4i8.v4i32(<4 x i32>)
define <4 x i32> @test_sat_dword_no_match(<4 x i32> %op0, <4 x i32> %op1, <4 x i32> %op2)  {
  ;CHECK: %1 = mul <4 x i32> %op0, %op1
  ;CHECK-NEXT: %2 = add <4 x i32> %1, %op2
  ;CHECK-NEXT: %3 = call <4 x i32> @llvm.genx.ustrunc.sat.v4i8.v4i32(<4 x i32> %2)
  %1 = mul <4 x i32> %op0, %op1
  %2 = add <4 x i32> %1, %op2
  %3 = call <4 x i32> @llvm.genx.ustrunc.sat.v4i8.v4i32(<4 x i32> %2)
  ret <4 x i32> %3
}

