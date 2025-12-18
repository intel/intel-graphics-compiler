;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s


; optimize
define <32 x i16> @test_match_v32i16(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)  {
  ; CHECK: [[V1:%[^ ]+]] = call <32 x i16> @llvm.genx.ssmad.v32i16.v32i16(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)

  %1 = mul <32 x i16> %op0, %op1
  %2 = add <32 x i16> %1, %op2

  ret <32 x i16> %2
}

; scalar value optimize
define i16 @test_match_i16(i16 %op0, i16 %op1, i16 %op2)  {
  ; CHECK: [[SIMPLE_RES:%[^ ]+]] = call i16 @llvm.genx.ssmad.i16.i16(i16 %op0, i16 %op1, i16 %op2)

  %1 = mul i16 %op0, %op1
  %2 = add i16 %1, %op2

  ret i16 %2
}
; float type
define <32 x float> @test_v32float(<32 x float> %op0, <32 x float> %op1, <32 x float> %op2)  {
  ;CHECK: [[RES:%[^ ]+]] = call <32 x float> @llvm.fma.v32f32(<32 x float> %op0, <32 x float> %op1, <32 x float> %op2)

  %1 = fmul <32 x float> %op0, %op1
  %2 = fadd <32 x float> %1, %op2

  ret <32 x float> %2
}
; scalar value negative
; place negative values before, after places ssmad
define i16 @test_match_i16_neg(i16 %op0, i16 %op1, i16 %op2)  {
  ; CHECK: [[NEG1:%[^ ]+]] = sub i16 0, %op2
  ; CHECK-NEXT: [[NEG_RES:%[^ ]+]] =  call i16 @llvm.genx.ssmad.i16.i16(i16 %op0, i16 %op1, i16 [[NEG1]])

  %1 = mul i16 %op0, %op1
  %2 = sub i16 %1, %op2

  ret i16 %2
}
; just check that it doesn't crash
define <32 x i16> @test_inline_asm(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)  {
  %1 = call <32 x i16> asm "mul (M1, 32) $0 $1 $2", "=r,r,r"(<32 x i16> %op0, <32 x i16> %op1)
  %2 = add <32 x i16> %1, %op2

  ret <32 x i16> %2
}
