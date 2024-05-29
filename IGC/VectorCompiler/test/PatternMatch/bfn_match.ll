;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-bfn=true -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define <32 x i16> @test_match_v32i16(<32 x i16> %op0, <32 x i16> %op1, <32 x i16> %op2)  {
; CHECK: call <32 x i16> @llvm.genx.bfn.v32i16.v32i16(
; CHECK-SAME-DAG: <32 x i16> %op0,
; CHECK-SAME-DAG: <32 x i16> %op1,
; CHECK-SAME-DAG: <32 x i16> %op2,
; CHECK-SAME: i8 -106)

  %1 = xor <32 x i16> %op0, %op1
  %2 = xor <32 x i16> %1, %op2

  ret <32 x i16> %2
}

define i32 @test_match_i32(i32 %op0, i32 %op1, i32 %op2)  {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(
; CHECK-SAME-DAG: i32 %op0,
; CHECK-SAME-DAG: i32 %op1,
; CHECK-SAME-DAG: i32 %op2,
; CHECK-SAME: i8 -128)

  %1 = and i32 %op0, %op1
  %2 = and i32 %1, %op2

  ret i32 %2
}

define <32 x i32> @test_match_v32i32xa(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
; CHECK: call <32 x i32> @llvm.genx.bfn.v32i32.v32i32(<32 x i32> %op2, <32 x i32> %op0, <32 x i32> %op1, i8 40)

  %1 = xor <32 x i32> %op0, %op1
  %2 = and <32 x i32> %1, %op2

  ret <32 x i32> %2
}

define <32 x i32> @test_match_v32i32xao(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
; CHECK: call <32 x i32> @llvm.genx.bfn.v32i32.v32i32(<32 x i32> %op1, <32 x i32> %op2, <32 x i32> %op0, i8 -82)

  %1 = xor <32 x i32> %op0, %op2
  %2 = and <32 x i32> %1, %op2
  %3 = or <32 x i32> %2, %op1

  ret <32 x i32> %3
}

define i32 @test_match_i32ox(i32 %op0, i32 %op1, i32 %op2)  {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %op2, i32 %op0, i32 %op1, i8 86)

  %1 = or i32 %op0, %op1
  %2 = xor i32 %op2, %1

  ret i32 %2
}
define i32 @test_match_i32oa(i32 %op0, i32 %op1, i32 %op2)  {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %op2, i32 %op1, i32 %op0, i8 -88)

  %1 = or i32 %op1, %op0
  %2 = and i32 %1, %op2

  ret i32 %2
}

define i32 @test_match_i32oo(i32 %op0, i32 %op1, i32 %op2)  {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %op2, i32 %op0, i32 %op1, i8 -2)

  %1 = or i32 %op0, %op1
  %2 = or i32 %1, %op2

  ret i32 %2
}

; R = (A & B) ^ (A & C) ^ (B & C)
define i32 @test_match_i32_xor_of_pair_and(i32 %a, i32 %b, i32 %c) {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %c, i32 %a, i32 %b, i8 -24)
  %1 = and i32 %a, %b
  %2 = and i32 %b, %c
  %3 = and i32 %c, %a
  %4 = xor i32 %1, %2
  %5 = xor i32 %4, %3
  ret i32 %5
}

define i32 @test_match_i32_combine_by_mask_inv(i32 %a, i32 %b, i32 %mask) {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %a, i32 %mask, i32 %b, i8 71)
  %nmask = xor i32 -1, %mask
  %1 = and i32 %a, %mask
  %2 = and i32 %b, %nmask
  %3 = or i32 %1, %2
  %4 = xor i32 %3, -1
  ret i32 %4
}

define i32 @test_match_i32_combine_by_mask_const(i32 %a, i32 %b) {
; CHECK: call i32 @llvm.genx.bfn.i32.i32(i32 %b, i32 %a, i32 4095, i8 -54)
  %1 = and i32 %a, 4095  ; 0x00000fff
  %2 = and i32 %b, -4096 ; 0xfffff000
  %3 = or i32 %2, %1
  ret i32 %3
}

declare void @use(i32)

; CHECK-LABEL: @test_unmatch_i32(
define i32 @test_unmatch_i32(i32 %op0, i32 %op1, i32 %op2)  {
; CHECK-NOT: call void @llvm.genx.bfn.i32.i32(
  %1 = and i32 %op0, %op1
  %2 = and i32 %1, %op2
  call void @use(i32 %1)

  ret i32 %2
}

declare i32 @llvm.genx.read.predef.reg.i32.i32(i32, i32)

; CHECK-LABEL: @test_unmatch_arf
define i32 @test_unmatch_arf(i32 %mask, i32 %src) {
; CHECK-NOT: call i32 @llvm.genx.bfn.i32.i32(
  %sr0 = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
  %1 = and i32 %sr0, %mask
  %2 = xor i32 %1, %src
  ret i32 %2
}

; CHECK-LABEL: @test_unmatch_flag
define i32 @test_unmatch_flag(<32 x i1> %a, <32 x i1> %b, <32 x i1> %c) {
  %as = bitcast <32 x i1> %a to i32
  %bs = bitcast <32 x i1> %b to i32
  %cs = bitcast <32 x i1> %c to i32

; CHECK-NOT: call i32 @llvm.genx.bfn.i32.i32(
  %1 = and i32 %as, %bs
  %2 = and i32 %bs, %cs

  ret i32 %2
}
