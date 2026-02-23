;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: packVecI32ToVecI64
; ------------------------------------------------
;
; Test checks that packing of two i32 elements from an <N x i32> vector into
; a single i64 is optimized to a bitcast + extractelement.
; Supports vectors with 2, 4, 6, or 8 i32 elements.
;
; Pattern matched:
;   %ee0 = extractelement <N x i32> %vec, i64 (2*k)
;   %ee1 = extractelement <N x i32> %vec, i64 (2*k+1)
;   %zext0 = zext i32 %ee0 to i64
;   %zext1 = zext i32 %ee1 to i64
;   %shl = shl i64 %zext1, 32
;   %or = or i64 %shl, %zext0
;
; Transformed to:
;   %bc = bitcast <N x i32> %vec to <N/2 x i64>
;   %result = extractelement <N/2 x i64> %bc, i64 k

; Test packing elements 0 and 1 into first i64
define i64 @test_pack_elements_0_1(<4 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_elements_0_1(
; CHECK-SAME: <4 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> [[VEC]] to <2 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i64> [[TMP1]], i64 0
; CHECK:    ret i64 [[TMP2]]
;
  %ee0 = extractelement <4 x i32> %vec, i64 0
  %ee1 = extractelement <4 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Test packing elements 2 and 3 into second i64
define i64 @test_pack_elements_2_3(<4 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_elements_2_3(
; CHECK-SAME: <4 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> [[VEC]] to <2 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i64> [[TMP1]], i64 1
; CHECK:    ret i64 [[TMP2]]
;
  %ee2 = extractelement <4 x i32> %vec, i64 2
  %ee3 = extractelement <4 x i32> %vec, i64 3
  %zext2 = zext i32 %ee2 to i64
  %zext3 = zext i32 %ee3 to i64
  %shl = shl i64 %zext3, 32
  %or = or i64 %shl, %zext2
  ret i64 %or
}

; Test with swapped or operands (commutative match)
define i64 @test_pack_commutative(<4 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_commutative(
; CHECK-SAME: <4 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> [[VEC]] to <2 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i64> [[TMP1]], i64 0
; CHECK:    ret i64 [[TMP2]]
;
  %ee0 = extractelement <4 x i32> %vec, i64 0
  %ee1 = extractelement <4 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %zext0, %shl
  ret i64 %or
}

; Negative test: wrong element indices (0, 2) - not a valid pair
define i64 @test_negative_wrong_indices(<4 x i32> %vec) {
; CHECK-LABEL: define i64 @test_negative_wrong_indices(
; CHECK-SAME: <4 x i32> [[VEC:%.*]]) {
; CHECK:    [[EE0:%.*]] = extractelement <4 x i32> [[VEC]], i64 0
; CHECK:    [[EE2:%.*]] = extractelement <4 x i32> [[VEC]], i64 2
; CHECK:    [[ZEXT0:%.*]] = zext i32 [[EE0]] to i64
; CHECK:    [[ZEXT2:%.*]] = zext i32 [[EE2]] to i64
; CHECK:    [[SHL:%.*]] = shl i64 [[ZEXT2]], 32
; CHECK:    [[OR:%.*]] = or i64 [[SHL]], [[ZEXT0]]
; CHECK:    ret i64 [[OR]]
;
  %ee0 = extractelement <4 x i32> %vec, i64 0
  %ee2 = extractelement <4 x i32> %vec, i64 2
  %zext0 = zext i32 %ee0 to i64
  %zext2 = zext i32 %ee2 to i64
  %shl = shl i64 %zext2, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Negative test: wrong shift amount (not 32)
define i64 @test_negative_wrong_shift(<4 x i32> %vec) {
; CHECK-LABEL: define i64 @test_negative_wrong_shift(
; CHECK-SAME: <4 x i32> [[VEC:%.*]]) {
; CHECK:    [[EE0:%.*]] = extractelement <4 x i32> [[VEC]], i64 0
; CHECK:    [[EE1:%.*]] = extractelement <4 x i32> [[VEC]], i64 1
; CHECK:    [[ZEXT0:%.*]] = zext i32 [[EE0]] to i64
; CHECK:    [[ZEXT1:%.*]] = zext i32 [[EE1]] to i64
; CHECK:    [[SHL:%.*]] = shl i64 [[ZEXT1]], 16
; CHECK:    [[OR:%.*]] = or i64 [[SHL]], [[ZEXT0]]
; CHECK:    ret i64 [[OR]]
;
  %ee0 = extractelement <4 x i32> %vec, i64 0
  %ee1 = extractelement <4 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 16
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Negative test: different source vectors
define i64 @test_negative_different_vectors(<4 x i32> %vec1, <4 x i32> %vec2) {
; CHECK-LABEL: define i64 @test_negative_different_vectors(
; CHECK-SAME: <4 x i32> [[VEC1:%.*]], <4 x i32> [[VEC2:%.*]]) {
; CHECK:    [[EE0:%.*]] = extractelement <4 x i32> [[VEC1]], i64 0
; CHECK:    [[EE1:%.*]] = extractelement <4 x i32> [[VEC2]], i64 1
; CHECK:    [[ZEXT0:%.*]] = zext i32 [[EE0]] to i64
; CHECK:    [[ZEXT1:%.*]] = zext i32 [[EE1]] to i64
; CHECK:    [[SHL:%.*]] = shl i64 [[ZEXT1]], 32
; CHECK:    [[OR:%.*]] = or i64 [[SHL]], [[ZEXT0]]
; CHECK:    ret i64 [[OR]]
;
  %ee0 = extractelement <4 x i32> %vec1, i64 0
  %ee1 = extractelement <4 x i32> %vec2, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Test <2 x i32> packing into <1 x i64>
define i64 @test_pack_2xi32(<2 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_2xi32(
; CHECK-SAME: <2 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i32> [[VEC]] to <1 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <1 x i64> [[TMP1]], i64 0
; CHECK:    ret i64 [[TMP2]]
;
  %ee0 = extractelement <2 x i32> %vec, i64 0
  %ee1 = extractelement <2 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Test <6 x i32> packing - elements 0,1
define i64 @test_pack_6xi32_0_1(<6 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_6xi32_0_1(
; CHECK-SAME: <6 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <6 x i32> [[VEC]] to <3 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <3 x i64> [[TMP1]], i64 0
; CHECK:    ret i64 [[TMP2]]
;
  %ee0 = extractelement <6 x i32> %vec, i64 0
  %ee1 = extractelement <6 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Test <6 x i32> packing - elements 4,5
define i64 @test_pack_6xi32_4_5(<6 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_6xi32_4_5(
; CHECK-SAME: <6 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <6 x i32> [[VEC]] to <3 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <3 x i64> [[TMP1]], i64 2
; CHECK:    ret i64 [[TMP2]]
;
  %ee4 = extractelement <6 x i32> %vec, i64 4
  %ee5 = extractelement <6 x i32> %vec, i64 5
  %zext4 = zext i32 %ee4 to i64
  %zext5 = zext i32 %ee5 to i64
  %shl = shl i64 %zext5, 32
  %or = or i64 %shl, %zext4
  ret i64 %or
}

; Test <8 x i32> packing - elements 0,1
define i64 @test_pack_8xi32_0_1(<8 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_8xi32_0_1(
; CHECK-SAME: <8 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <8 x i32> [[VEC]] to <4 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[TMP1]], i64 0
; CHECK:    ret i64 [[TMP2]]
;
  %ee0 = extractelement <8 x i32> %vec, i64 0
  %ee1 = extractelement <8 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Test <8 x i32> packing - elements 6,7
define i64 @test_pack_8xi32_6_7(<8 x i32> %vec) {
; CHECK-LABEL: define i64 @test_pack_8xi32_6_7(
; CHECK-SAME: <8 x i32> [[VEC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <8 x i32> [[VEC]] to <4 x i64>
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[TMP1]], i64 3
; CHECK:    ret i64 [[TMP2]]
;
  %ee6 = extractelement <8 x i32> %vec, i64 6
  %ee7 = extractelement <8 x i32> %vec, i64 7
  %zext6 = zext i32 %ee6 to i64
  %zext7 = zext i32 %ee7 to i64
  %shl = shl i64 %zext7, 32
  %or = or i64 %shl, %zext6
  ret i64 %or
}

; Negative test: odd number of elements (not supported)
define i64 @test_negative_odd_elements(<3 x i32> %vec) {
; CHECK-LABEL: define i64 @test_negative_odd_elements(
; CHECK-SAME: <3 x i32> [[VEC:%.*]]) {
; CHECK:    [[EE0:%.*]] = extractelement <3 x i32> [[VEC]], i64 0
; CHECK:    [[EE1:%.*]] = extractelement <3 x i32> [[VEC]], i64 1
; CHECK:    [[ZEXT0:%.*]] = zext i32 [[EE0]] to i64
; CHECK:    [[ZEXT1:%.*]] = zext i32 [[EE1]] to i64
; CHECK:    [[SHL:%.*]] = shl i64 [[ZEXT1]], 32
; CHECK:    [[OR:%.*]] = or i64 [[SHL]], [[ZEXT0]]
; CHECK:    ret i64 [[OR]]
;
  %ee0 = extractelement <3 x i32> %vec, i64 0
  %ee1 = extractelement <3 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}

; Negative test: vector too large (>8 elements)
define i64 @test_negative_too_large(<16 x i32> %vec) {
; CHECK-LABEL: define i64 @test_negative_too_large(
; CHECK-SAME: <16 x i32> [[VEC:%.*]]) {
; CHECK:    [[EE0:%.*]] = extractelement <16 x i32> [[VEC]], i64 0
; CHECK:    [[EE1:%.*]] = extractelement <16 x i32> [[VEC]], i64 1
; CHECK:    [[ZEXT0:%.*]] = zext i32 [[EE0]] to i64
; CHECK:    [[ZEXT1:%.*]] = zext i32 [[EE1]] to i64
; CHECK:    [[SHL:%.*]] = shl i64 [[ZEXT1]], 32
; CHECK:    [[OR:%.*]] = or i64 [[SHL]], [[ZEXT0]]
; CHECK:    ret i64 [[OR]]
;
  %ee0 = extractelement <16 x i32> %vec, i64 0
  %ee1 = extractelement <16 x i32> %vec, i64 1
  %zext0 = zext i32 %ee0 to i64
  %zext1 = zext i32 %ee1 to i64
  %shl = shl i64 %zext1, 32
  %or = or i64 %shl, %zext0
  ret i64 %or
}
