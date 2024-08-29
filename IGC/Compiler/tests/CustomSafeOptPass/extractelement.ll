;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: extractelement + bitcast
; ------------------------------------------------
;
; Test checks that sequence:
; %1 = lshr i32 %0, 16,
; %2 = bitcast i32 %1 to <2 x half>
; %3 = extractelement <2 x half> %2, i32 0
; is converted to ->
; %2 = bitcast i32 %0 to <2 x half>
; %3 = extractelement <2 x half> %2, i32 1

define i16 @test_extract_lshr16(i32 %src1) {
; CHECK-LABEL: define i16 @test_extract_lshr16(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = lshr i32 [[SRC1]], 16
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <2 x i16>
; CHECK:    [[TMP1:%.*]] = bitcast i32 [[SRC1]] to <2 x i16>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i16> [[TMP1]], i64 1
; CHECK:    ret i16 [[TMP2]]
;
  %1 = lshr i32 %src1, 16
  %2 = bitcast i32 %1 to <2 x i16>
  %3 = extractelement <2 x i16> %2, i32 0
  ret i16 %3
}

define i16 @test_extract_lshl16(i32 %src1) {
; CHECK-LABEL: define i16 @test_extract_lshl16(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = shl i32 [[SRC1]], 16
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <2 x i16>
; CHECK:    [[TMP1:%.*]] = bitcast i32 [[SRC1]] to <2 x i16>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i16> [[TMP1]], i64 0
; CHECK:    ret i16 [[TMP2]]
;
  %1 = shl i32 %src1, 16
  %2 = bitcast i32 %1 to <2 x i16>
  %3 = extractelement <2 x i16> %2, i32 1
  ret i16 %3
}

define i8 @test_extract_lshr8(i32 %src1) {
; CHECK-LABEL: define i8 @test_extract_lshr8(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = lshr i32 [[SRC1]], 16
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <4 x i8>
; CHECK:    [[TMP1:%.*]] = bitcast i32 [[SRC1]] to <4 x i8>
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i8> [[TMP1]], i64 2
; CHECK:    ret i8 [[TMP2]]
;
  %1 = lshr i32 %src1, 16
  %2 = bitcast i32 %1 to <4 x i8>
  %3 = extractelement <4 x i8> %2, i32 0
  ret i8 %3
}

define i8 @test_extract_lshl8(i32 %src1) {
; CHECK-LABEL: define i8 @test_extract_lshl8(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = shl i32 [[SRC1]], 16
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <4 x i8>
; CHECK:    [[TMP1:%.*]] = bitcast i32 [[SRC1]] to <4 x i8>
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i8> [[TMP1]], i64 0
; CHECK:    ret i8 [[TMP2]]
;
  %1 = shl i32 %src1, 16
  %2 = bitcast i32 %1 to <4 x i8>
  %3 = extractelement <4 x i8> %2, i32 2
  ret i8 %3
}

define i1 @test_extract_lshr1(i8 %src1) {
; CHECK-LABEL: define i1 @test_extract_lshr1(
; CHECK-SAME: i8 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = lshr i8 [[SRC1]], 1
; CHECK:    [[TMP4:%.*]] = bitcast i8 [[TMP3]] to <8 x i1>
; CHECK:    [[TMP1:%.*]] = bitcast i8 [[SRC1]] to <8 x i1>
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i1> [[TMP1]], i64 7
; CHECK:    ret i1 [[TMP2]]
;
  %1 = lshr i8 %src1, 1
  %2 = bitcast i8 %1 to <8 x i1>
  %3 = extractelement <8 x i1> %2, i32 6
  ret i1 %3
}

define i1 @test_extract_lshl1(i8 %src1) {
; CHECK-LABEL: define i1 @test_extract_lshl1(
; CHECK-SAME: i8 [[SRC1:%.*]]) {
; CHECK:    [[TMP3:%.*]] = shl i8 [[SRC1]], 4
; CHECK:    [[TMP4:%.*]] = bitcast i8 [[TMP3]] to <8 x i1>
; CHECK:    [[TMP1:%.*]] = bitcast i8 [[SRC1]] to <8 x i1>
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i1> [[TMP1]], i64 3
; CHECK:    ret i1 [[TMP2]]
;
  %1 = shl i8 %src1, 4
  %2 = bitcast i8 %1 to <8 x i1>
  %3 = extractelement <8 x i1> %2, i32 7
  ret i1 %3
}

; Negative cases:
; Trying to get zero parts is not optimized by this pass now
;
define i1 @test_extract_lshr_negative(i8 %src1) {
; CHECK-LABEL: define i1 @test_extract_lshr_negative(
; CHECK-SAME: i8 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = lshr i8 [[SRC1]], 7
; CHECK:    [[TMP2:%.*]] = bitcast i8 [[TMP1]] to <8 x i1>
; CHECK:    [[TMP3:%.*]] = extractelement <8 x i1> [[TMP2]], i32 1
; CHECK:    ret i1 [[TMP3]]
;
  %1 = lshr i8 %src1, 7
  %2 = bitcast i8 %1 to <8 x i1>
  %3 = extractelement <8 x i1> %2, i32 1
  ret i1 %3
}

define i1 @test_extract_lshl_negative(i8 %src1) {
; CHECK-LABEL: define i1 @test_extract_lshl_negative(
; CHECK-SAME: i8 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i8 [[SRC1]], 4
; CHECK:    [[TMP2:%.*]] = bitcast i8 [[TMP1]] to <8 x i1>
; CHECK:    [[TMP3:%.*]] = extractelement <8 x i1> [[TMP2]], i32 3
; CHECK:    ret i1 [[TMP3]]
;
  %1 = shl i8 %src1, 4
  %2 = bitcast i8 %1 to <8 x i1>
  %3 = extractelement <8 x i1> %2, i32 3
  ret i1 %3
}
