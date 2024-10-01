;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-gen-specific-pattern -S -dce < %s | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: shl + or
; ------------------------------------------------

; More cases for shl + or pattern

define i64 @test_shl_or_max(i64 %src1) {
; CHECK-LABEL: define i64 @test_shl_or_max(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i64 [[SRC1]], 63
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 9223372036854775807
; CHECK:    ret i64 [[TMP2]]
;
  %1 = shl i64 %src1, 63
  %2 = or i64 %1, 9223372036854775807 ; 0x7FFFFFFFFFFFFFFF
  ret i64 %2
}

define i64 @test_shl_or_cond(i64 %src1) {
; CHECK-LABEL: define i64 @test_shl_or_cond(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i64 [[SRC1]], 62
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 4611686018427387648
; CHECK:    ret i64 [[TMP2]]
;
  %1 = shl i64 %src1, 62
  %2 = or i64 %1, 4611686018427387648 ; 0x3FFFFFFFFFFFFF00
  ret i64 %2
}

; Negative cases(not optimized)
define i64 @test_shl_or_n1(i64 %src1) {
; CHECK-LABEL: define i64 @test_shl_or_n1(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i64 [[SRC1]], 62
; CHECK:    [[TMP2:%.*]] = or i64 [[TMP1]], 4611686018427387904
; CHECK:    ret i64 [[TMP2]]
;
  %1 = shl i64 %src1, 62
  %2 = or i64 %1, 4611686018427387904 ; 0x4000000000000000
  ret i64 %2
}

define i64 @test_shl_or_n2(i64 %src1) {
; CHECK-LABEL: define i64 @test_shl_or_n2(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i64 [[SRC1]], 63
; CHECK:    [[TMP2:%.*]] = or i64 [[TMP1]], -9223372036854775808
; CHECK:    ret i64 [[TMP2]]
;
  %1 = shl i64 %src1, 63
  %2 = or i64 %1, -9223372036854775808 ; 0x8000000000000000
  ret i64 %2
}
