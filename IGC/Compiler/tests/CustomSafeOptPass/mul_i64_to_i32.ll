;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: And(lower64bto32b)
; ------------------------------------------------

; Test check that
;
; %binop = binop i64 %src1, %src2
; %1 = and i64 %binop, 0xfffffff
;
; is substituted with
;
; %1 = trunc i64 %src1 to i32
; %2 = trunc i64 %src2 to i32
; %3 = binop i32 %1, %2
; %4 = zext i32 %3 to i64

; For this binops: Add, Sub, Mul, And, Or, Xor

define i64 @test_lower_add(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_add(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = add i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

define i64 @test_lower_sub(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_sub(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = sub i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = sub i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

define i64 @test_lower_mul(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_mul(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = mul i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

define i64 @test_lower_and(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_and(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = and i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = and i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

define i64 @test_lower_or(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_or(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = or i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = or i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

define i64 @test_lower_xor(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_xor(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = trunc i64 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = xor i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK:    ret i64 [[TMP4]]
;
  %1 = xor i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

; negative case, udiv shouldn't be optimized

define i64 @test_not_lower_udiv(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_not_lower_udiv(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = udiv i64 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP2:%.*]] = and i64 [[TMP1]], 4294967295
; CHECK:    ret i64 [[TMP2]]
;
  %1 = udiv i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  ret i64 %2
}

; negative case, multiple uses:

define i64 @test_lower_multiple(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_multiple(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i64 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP2:%.*]] = and i64 [[TMP1]], 4294967295
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK:    ret i64 [[TMP3]]
;
  %1 = mul i64 %src1, %src2
  %2 = and i64 %1, 4294967295
  %3 = add i64 %1, %2
  ret i64 %3
}

; longer sequence of binary ops:

define i64 @test_lower_sequence(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_lower_sequence(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i64 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP2:%.*]] = add i64 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP3:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[TMP4:%.*]] = trunc i64 [[TMP2]] to i32
; CHECK:    [[TMP5:%.*]] = sub i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[TMP7:%.*]] = trunc i64 [[TMP2]] to i32
; CHECK:    [[TMP8:%.*]] = or i32 [[TMP6]], [[TMP7]]
; CHECK:    [[TMP9:%.*]] = and i32 [[TMP5]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = zext i32 [[TMP9]] to i64
; CHECK:    ret i64 [[TMP10]]
;
  %1 = mul i64 %src1, %src2
  %2 = add i64 %src1, %src2
  %3 = sub i64 %1, %2
  %4 = or  i64 %1, %2
  %5 = and i64 %3, %4
  %6 = and i64 %5, 4294967295
  ret i64 %6
}
