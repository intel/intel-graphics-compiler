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
; Legalization: bitcast to illegal type
; ------------------------------------------------

; Checks legalization of these patterns:
;
; 1.
; %a = bitcast <3 x half> %src to i48
; %trunc = trunc i48 %a to i16
; %bitcast = bitcast i16 %trunc to half
;
; into
;
; %bitcast = extract <3 x half> %src, i32 0
;
; 2.
; %a = bitcast <3 x half> %src to i48
; %b = lshr i48 %a, 16
; %trunc = trunc i48 %b to i16
; %bitcast = bitcast i16 %trunc to half
;
; into
;
; %bitcast = extract <3 x half> %src, i32 1

define i16 @test_bitcast_pattern1(<5 x i16> %src1) {
; CHECK-LABEL: define i16 @test_bitcast_pattern1(
; CHECK-SAME: <5 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i16> [[SRC1]], i32 0
; CHECK:    ret i16 [[TMP1]]
;
  %1 = bitcast <5 x i16> %src1 to i80
  %2 = trunc i80 %1 to i16
  ret i16 %2
}

define i16 @test_bitcast_pattern1_lshr(<5 x i16> %src1) {
; CHECK-LABEL: define i16 @test_bitcast_pattern1_lshr(
; CHECK-SAME: <5 x i16> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i16> [[SRC1]], i32 2
; CHECK:    ret i16 [[TMP1]]
;
  %1 = bitcast <5 x i16> %src1 to i80
  %2 = lshr i80 %1, 32
  %3 = trunc i80 %2 to i16
  ret i16 %3
}

define i16 @test_bitcast_pattern1_half(<5 x half> %src1) {
; CHECK-LABEL: define i16 @test_bitcast_pattern1_half(
; CHECK-SAME: <5 x half> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <5 x half> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = bitcast half [[TMP1]] to i16
; CHECK:    ret i16 [[TMP2]]
;
  %1 = bitcast <5 x half> %src1 to i80
  %2 = trunc i80 %1 to i16
  ret i16 %2
}

define i1 @test_bitcast_pattern1_lshr_i1(<4 x i1> %src1) {
; CHECK-LABEL: define i1 @test_bitcast_pattern1_lshr_i1(
; CHECK-SAME: <4 x i1> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i1> [[SRC1]], i32 1
; CHECK:    ret i1 [[TMP1]]
;
  %1 = bitcast <4 x i1> %src1 to i4
  %2 = lshr i4 %1, 1
  %3 = trunc i4 %2 to i1
  ret i1 %3
}

define half @test_bitcast_pattern1_cast(<5 x half> %src1) {
; CHECK-LABEL: define half @test_bitcast_pattern1_cast(
; CHECK-SAME: <5 x half> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <5 x half> [[SRC1]], i32 0
; CHECK:    ret half [[TMP1]]
;
  %1 = bitcast <5 x half> %src1 to i80
  %2 = trunc i80 %1 to i16
  %3 = bitcast i16 %2 to half
  ret half %3
}

!igc.functions = !{!0, !1, !2, !3, !4}

!0 = !{i16 (<5 x i16>)* @test_bitcast_pattern1, !5}
!1 = !{i16 (<5 x i16>)* @test_bitcast_pattern1_lshr, !5}
!2 = !{i16 (<5 x half>)* @test_bitcast_pattern1_half, !5}
!3 = !{i1 (<4 x i1>)* @test_bitcast_pattern1_lshr_i1, !5}
!4 = !{half (<5 x half>)* @test_bitcast_pattern1_cast, !5}
!5 = !{}
