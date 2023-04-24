;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch --enable-bfn=true -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: check cm/cm_bfn.h header for more information about bfn intrinsic

; optimize
; AA ^ CC ^ F0 = 0x96 = -106
define <32 x i32> @test_match_v32i32(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
  ;CHECK: [[V1:%[^ ]+]] = call <32 x i32> @llvm.genx.bfn.v32i32.v32i32(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2, i8 -106)

  %1 = xor <32 x i32> %op0, %op1
  %2 = xor <32 x i32> %1, %op2

  ret <32 x i32> %2
}

; scalar value optimize
; AA & CC & F0 = 0x80 = -128
define i32 @test_match_i32(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[SIMPLE_RES:%[^ ]+]] = call i32 @llvm.genx.bfn.i32.i32(i32 %op0, i32 %op1, i32 %op2, i8 -128)

  %1 = and i32 %op0, %op1
  %2 = and i32 %1, %op2

  ret i32 %2
}
define <32 x i32> @test_match_v32i32xa(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
  ;CHECK: [[V1:%[^ ]+]] = call <32 x i32> @llvm.genx.bfn.v32i32.v32i32(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2, i8 96)

  %1 = xor <32 x i32> %op0, %op1
  %2 = and <32 x i32> %1, %op2

  ret <32 x i32> %2
}
; in result first match, other not
define <32 x i32> @test_match_v32i32xao(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
  ;CHECK: [[V1:%[^ ]+]] = call <32 x i32> @llvm.genx.bfn.v32i32.v32i32(<32 x i32> %op0, <32 x i32> %op2, <32 x i32> %op2, i8 96)

  %1 = xor <32 x i32> %op0, %op2
  %2 = and <32 x i32> %1, %op2
  %3 = or <32 x i32> %2, %op1

  ret <32 x i32> %3
}
define i32 @test_match_i32ox(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[SIMPLE_RES:%[^ ]+]] = call i32 @llvm.genx.bfn.i32.i32(i32 %op0, i32 %op1, i32 %op2, i8 30)

  %1 = or i32 %op0, %op1
  %2 = xor i32 %op2, %1

  ret i32 %2
}
define i32 @test_match_i32oa(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[SIMPLE_RES:%[^ ]+]] = call i32 @llvm.genx.bfn.i32.i32(i32 %op1, i32 %op0, i32 %op2, i8 -32)

  %1 = or i32 %op1, %op0
  %2 = and i32 %1, %op2

  ret i32 %2
}
define i32 @test_match_i32oo(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[SIMPLE_RES:%[^ ]+]] = call i32 @llvm.genx.bfn.i32.i32(i32 %op0, i32 %op1, i32 %op2, i8 -2)

  %1 = or i32 %op0, %op1
  %2 = or i32 %1, %op2

  ret i32 %2
}
