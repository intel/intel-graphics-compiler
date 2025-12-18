;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=NoMadDDQ %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32>, <2 x i32>)

define <2 x i64> @test_ssmul_v2i64_v2i32_add_v2i64_1(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = add <2 x i64> %op2, %mul
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.ssmad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = add <2 x i64> %op2, %mul
  ret <2 x i64> %res
}

define <2 x i64> @test_ssmul_v2i64_v2i32_add_v2i64_2(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = add <2 x i64> %mul, %op2
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.ssmad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = add <2 x i64> %mul, %op2
  ret <2 x i64> %res
}

define <2 x i64> @test_ssmul_v2i64_v2i32_sub_v2i64_1(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = sub <2 x i64> %op2, %mul
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[NEG:%[^ ]+]] = sub <2 x i32> zeroinitializer, %op1
  ; CHECK-NEXT: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.ssmad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> [[NEG]], <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = sub <2 x i64> %op2, %mul
  ret <2 x i64> %res
}

define <2 x i64> @test_ssmul_v2i64_v2i32_sub_v2i64_2(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = sub <2 x i64> %mul, %op2
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[NEG:%[^ ]+]] = sub <2 x i64> zeroinitializer, %op2
  ; CHECK-NEXT: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.ssmad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> [[NEG]])
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = sub <2 x i64> %mul, %op2
  ret <2 x i64> %res
}

define <2 x i64> @test_uumul_v2i64_v2i32_add_v2i64_1(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = add <2 x i64> %op2, %mul
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.uumad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = add <2 x i64> %op2, %mul
  ret <2 x i64> %res
}

define <2 x i64> @test_uumul_v2i64_v2i32_add_v2i64_2(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = add <2 x i64> %mul, %op2
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.uumad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = add <2 x i64> %mul, %op2
  ret <2 x i64> %res
}

define <2 x i64> @test_uumul_v2i64_v2i32_sub_v2i64_1(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = sub <2 x i64> %op2, %mul
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[NEG:%[^ ]+]] = sub <2 x i32> zeroinitializer, %op1
  ; CHECK-NEXT: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.uumad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> [[NEG]], <2 x i64> %op2)
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = sub <2 x i64> %op2, %mul
  ret <2 x i64> %res
}

define <2 x i64> @test_uumul_v2i64_v2i32_sub_v2i64_2(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> %op2)  {
  ; NoMadDDQ: %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  ; NoMadDDQ-NEXT: %res = sub <2 x i64> %mul, %op2
  ; NoMadDDQ-NEXT: ret <2 x i64> %res
  ; CHECK: [[NEG:%[^ ]+]] = sub <2 x i64> zeroinitializer, %op2
  ; CHECK-NEXT: [[MAD:%[^ ]+]] = call <2 x i64> @llvm.genx.uumad.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i64> [[NEG]])
  ; CHECK-NEXT: ret <2 x i64> [[MAD]]
  %mul = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op0, <2 x i32> %op1)
  %res = sub <2 x i64> %mul, %op2
  ret <2 x i64> %res
}
