;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-mad=true -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-MULI32 %s

define <16 x i32> @test_mul_add_i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i32
; CHECK: %mul = mul <16 x i32> %op0, %op1
; CHECK: %add = add <16 x i32> %op2, %mul
; CHECK-MULI32-LABEL: @test_mul_add_i32
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2)
  %mul = mul <16 x i32> %op0, %op1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_radd_i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_radd_i32
; CHECK: %mul = mul <16 x i32> %op0, %op1
; CHECK: %add = add <16 x i32> %mul, %op2
; CHECK-MULI32-LABEL: @test_mul_radd_i32
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2)
  %mul = mul <16 x i32> %op0, %op1
  %add = add <16 x i32> %mul, %op2
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_sub_i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_sub_i32
; CHECK: %mul = mul <16 x i32> %op0, %op1
; CHECK: %sub = sub <16 x i32> %op2, %mul
; CHECK-MULI32-LABEL: @test_mul_sub_i32
; CHECK-MULI32: [[NEG:%[^ ]+]] = sub <16 x i32> zeroinitializer, %op1
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %op0, <16 x i32> [[NEG]], <16 x i32> %op2)
  %mul = mul <16 x i32> %op0, %op1
  %sub = sub <16 x i32> %op2, %mul
  ret <16 x i32> %sub
}

define <16 x i32> @test_mul_rsub_i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_rsub_i32
; CHECK: %mul = mul <16 x i32> %op0, %op1
; CHECK: %sub = sub <16 x i32> %mul, %op2
; CHECK-MULI32-LABEL: @test_mul_rsub_i32
; CHECK-MULI32: [[NEG:%[^ ]+]] = sub <16 x i32> zeroinitializer, %op2
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> [[NEG]])
  %mul = mul <16 x i32> %op0, %op1
  %sub = sub <16 x i32> %mul, %op2
  ret <16 x i32> %sub
}

define <16 x i32> @test_mul_add_i16_zext(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_zext
; CHECK: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2)
  %ext0 = zext <16 x i16> %op0 to <16 x i32>
  %ext1 = zext <16 x i16> %op1 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %ext1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_sext(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_sext
; CHECK: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2)
  %ext0 = sext <16 x i16> %op0 to <16 x i32>
  %ext1 = sext <16 x i16> %op1 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %ext1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_szext(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_szext
; CHECK: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2)
  %ext0 = sext <16 x i16> %op0 to <16 x i32>
  %ext1 = zext <16 x i16> %op1 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %ext1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_zsext(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_zsext
; CHECK: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> %op0, <16 x i16> %op1, <16 x i32> %op2)
  %ext0 = zext <16 x i16> %op0 to <16 x i32>
  %ext1 = sext <16 x i16> %op1 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %ext1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_zext_op0(<16 x i16> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_zext_op0
; CHECK: %ext0 = zext <16 x i16> %op0 to <16 x i32>
; CHECK: %mul = mul <16 x i32> %ext0, %op1
; CHECK: %add = add <16 x i32> %op2, %mul
; CHECK-MULI32-LABEL: @test_mul_add_i16_zext_op0
; CHECK-MULI32: %ext0 = zext <16 x i16> %op0 to <16 x i32>
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %ext0, <16 x i32> %op1, <16 x i32> %op2)
  %ext0 = zext <16 x i16> %op0 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %op1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_sext_op0(<16 x i16> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_sext_op0
; CHECK: %ext0 = sext <16 x i16> %op0 to <16 x i32>
; CHECK: %mul = mul <16 x i32> %ext0, %op1
; CHECK: %add = add <16 x i32> %op2, %mul
; CHECK-MULI32-LABEL: @test_mul_add_i16_sext_op0
; CHECK-MULI32: %ext0 = sext <16 x i16> %op0 to <16 x i32>
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %ext0, <16 x i32> %op1, <16 x i32> %op2)
  %ext0 = sext <16 x i16> %op0 to <16 x i32>
  %mul = mul <16 x i32> %ext0, %op1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}

define <16 x i32> @test_mul_add_i16_sext_op1(<16 x i32> %op0, <16 x i16> %op1, <16 x i32> %op2) {
; CHECK-LABEL: @test_mul_add_i16_sext_op1
; CHECK: %ext1 = sext <16 x i16> %op1 to <16 x i32>
; CHECK: %mul = mul <16 x i32> %op0, %ext1
; CHECK: %add = add <16 x i32> %op2, %mul
; CHECK-MULI32-LABEL: @test_mul_add_i16_sext_op1
; CHECK-MULI32: %ext1 = sext <16 x i16> %op1 to <16 x i32>
; CHECK-MULI32: [[MAD:%[^ ]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> %op0, <16 x i32> %ext1, <16 x i32> %op2)
  %ext1 = sext <16 x i16> %op1 to <16 x i32>
  %mul = mul <16 x i32> %op0, %ext1
  %add = add <16 x i32> %op2, %mul
  ret <16 x i32> %add
}
