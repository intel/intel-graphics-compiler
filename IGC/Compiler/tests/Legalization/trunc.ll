;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: trunc
; ------------------------------------------------

; Checks legalization trunc of i48 to i32

define i32 @test_trunc(<3 x i16> %src1) {
; CHECK-LABEL: define i32 @test_trunc(
; CHECK-SAME: <3 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <3 x i16> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = zext i16 [[TMP1]] to i32
; CHECK:    [[TMP3:%.*]] = extractelement <3 x i16> [[SRC1]], i32 1
; CHECK:    [[TMP4:%.*]] = zext i16 [[TMP3]] to i32
; CHECK:    [[TMP5:%.*]] = shl i32 [[TMP4]], 16
; CHECK:    [[TMP6:%.*]] = or i32 [[TMP5]], [[TMP2]]
; CHECK:    ret i32 [[TMP6]]
;
  %1 = bitcast <3 x i16> %src1 to i48
  %2 = trunc i48 %1 to i32
  ret i32 %2
}

define i32 @test_trunc_shr16(<3 x i16> %src1) {
; CHECK-LABEL: define i32 @test_trunc_shr16(
; CHECK-SAME: <3 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <3 x i16> [[SRC1]], i32 1
; CHECK:    [[TMP2:%.*]] = zext i16 [[TMP1]] to i32
; CHECK:    [[TMP3:%.*]] = extractelement <3 x i16> [[SRC1]], i32 2
; CHECK:    [[TMP4:%.*]] = zext i16 [[TMP3]] to i32
; CHECK:    [[TMP5:%.*]] = shl i32 [[TMP4]], 16
; CHECK:    [[TMP6:%.*]] = or i32 [[TMP5]], [[TMP2]]
; CHECK:    ret i32 [[TMP6]]
;
  %1 = bitcast <3 x i16> %src1 to i48
  %2 = lshr i48 %1, 16
  %3 = trunc i48 %2 to i32
  ret i32 %3
}

define i32 @test_trunc_shr32(<3 x i16> %src1) {
; CHECK-LABEL: define i32 @test_trunc_shr32(
; CHECK-SAME: <3 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <3 x i16> [[SRC1]], i32 2
; CHECK:    [[TMP2:%.*]] = zext i16 [[TMP1]] to i32
; CHECK:    ret i32 [[TMP2]]
;
  %1 = bitcast <3 x i16> %src1 to i48
  %2 = lshr i48 %1, 32
  %3 = trunc i48 %2 to i32
  ret i32 %3
}

; Negative cases

; ashr
define i32 @test_trunc_ashr(<3 x i16> %src1) {
; CHECK-LABEL: define i32 @test_trunc_ashr(
; CHECK-SAME: <3 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <3 x i16> [[SRC1]] to i48
; CHECK:    [[TMP2:%.*]] = ashr i48 [[TMP1]], 16
; CHECK:    [[TMP3:%.*]] = trunc i48 [[TMP2]] to i32
; CHECK:    ret i32 [[TMP3]]
;
  %1 = bitcast <3 x i16> %src1 to i48
  %2 = ashr i48 %1, 16
  %3 = trunc i48 %2 to i32
  ret i32 %3
}

; non 16 or 32 shift

define i32 @test_trunc_lshr8(<3 x i16> %src1) {
; CHECK-LABEL: define i32 @test_trunc_lshr8(
; CHECK-SAME: <3 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <3 x i16> [[SRC1]] to i48
; CHECK:    [[TMP2:%.*]] = lshr i48 [[TMP1]], 8
; CHECK:    [[TMP3:%.*]] = trunc i48 [[TMP2]] to i32
; CHECK:    ret i32 [[TMP3]]
;
  %1 = bitcast <3 x i16> %src1 to i48
  %2 = lshr i48 %1, 8
  %3 = trunc i48 %2 to i32
  ret i32 %3
}

!igc.functions = !{!0, !1, !2, !3, !4}

!0 = !{i32 (<3 x i16>)* @test_trunc, !10}
!1 = !{i32 (<3 x i16>)* @test_trunc_shr16, !10}
!2 = !{i32 (<3 x i16>)* @test_trunc_shr32, !10}
!3 = !{i32 (<3 x i16>)* @test_trunc_ashr, !10}
!4 = !{i32 (<3 x i16>)* @test_trunc_lshr8, !10}
!10 = !{}
