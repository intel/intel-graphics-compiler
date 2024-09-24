;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: And and OR instructions
; ------------------------------------------------
;
; Pass checks that
; (!a && !b) is transformed to !(a || b)
; and
; (!a || !b) to !(a && b)

define i32 @test_and_select(i1 %src1, i1 %src2) {
; CHECK-LABEL: define i32 @test_and_select(
; CHECK-SAME: i1 [[SRC1:%.*]], i1 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = or i1 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP2:%.*]] = select i1 [[TMP1]], i32 144, i32 13
; CHECK:    ret i32 [[TMP2]]
;
  %1 = xor i1 %src1, true
  %2 = xor i1 %src2, true
  %3 = and i1 %1, %2
  %4 = select i1 %3, i32 13, i32 144
  ret i32 %4
}

define i32 @test_and_branch(i1 %src1, i1 %src2) {
; CHECK-LABEL: define i32 @test_and_branch(
; CHECK-SAME: i1 [[SRC1:%.*]], i1 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = or i1 [[SRC1]], [[SRC2]]
; CHECK:    br i1 [[TMP1]], label %[[BLOCK2:.*]], label %[[BLOCK1:.*]]
; CHECK:       [[BLOCK1]]:
; CHECK:    br label %[[UNIFIEDRETURNBLOCK:.*]]
; CHECK:       [[BLOCK2]]:
; CHECK:    br label %[[UNIFIEDRETURNBLOCK]]
; CHECK:       [[UNIFIEDRETURNBLOCK]]:
; CHECK:    [[UNIFIEDRETVAL:%.*]] = phi i32 [ 13, %[[BLOCK1]] ], [ 144, %[[BLOCK2]] ]
; CHECK:    ret i32 [[UNIFIEDRETVAL]]
;
  %1 = xor i1 %src1, true
  %2 = xor i1 %src2, true
  %3 = and i1 %1, %2
  br i1 %3, label %block1, label %block2
block1:
  ret i32 13
block2:
  ret i32 144
}

define i32 @test_or_select(i1 %src1, i1 %src2) {
; CHECK-LABEL: define i32 @test_or_select(
; CHECK-SAME: i1 [[SRC1:%.*]], i1 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = and i1 [[SRC1]], [[SRC2]]
; CHECK:    [[TMP2:%.*]] = select i1 [[TMP1]], i32 144, i32 13
; CHECK:    ret i32 [[TMP2]]
;
  %1 = xor i1 %src1, true
  %2 = xor i1 %src2, true
  %3 = or i1 %1, %2
  %4 = select i1 %3, i32 13, i32 144
  ret i32 %4
}

define i32 @test_or_branch(i1 %src1, i1 %src2) {
; CHECK-LABEL: define i32 @test_or_branch(
; CHECK-SAME: i1 [[SRC1:%.*]], i1 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = or i1 [[SRC1]], [[SRC2]]
; CHECK:    br i1 [[TMP1]], label %[[BLOCK2:.*]], label %[[BLOCK1:.*]]
; CHECK:       [[BLOCK1]]:
; CHECK:    br label %[[UNIFIEDRETURNBLOCK:.*]]
; CHECK:       [[BLOCK2]]:
; CHECK:    br label %[[UNIFIEDRETURNBLOCK]]
; CHECK:       [[UNIFIEDRETURNBLOCK]]:
; CHECK:    [[UNIFIEDRETVAL:%.*]] = phi i32 [ 13, %[[BLOCK1]] ], [ 144, %[[BLOCK2]] ]
; CHECK:    ret i32 [[UNIFIEDRETVAL]]
;
  %1 = xor i1 %src1, true
  %2 = xor i1 %src2, true
  %3 = and i1 %1, %2
  br i1 %3, label %block1, label %block2
block1:
  ret i32 13
block2:
  ret i32 144
}

; Negative case, and result on selected value
define i32 @test_and_select_negative(i1 %cc, i32 %src1, i32 %src2) {
; CHECK-LABEL: define i32 @test_and_select_negative(
; CHECK-SAME: i1 [[CC:%.*]], i32 [[SRC1:%.*]], i32 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = xor i32 [[SRC1]], -1
; CHECK:    [[TMP2:%.*]] = xor i32 [[SRC2]], -1
; CHECK:    [[TMP3:%.*]] = and i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 144
; CHECK:    ret i32 [[TMP4]]
;
  %1 = xor i32 %src1, -1
  %2 = xor i32 %src2, -1
  %3 = and i32 %1, %2
  %4 = select i1 %cc, i32 %3, i32 144
  ret i32 %4
}

!igc.functions = !{!0, !1, !2, !3, !4}
!0 = !{i32 (i1, i1)* @test_and_select, !6}
!1 = !{i32 (i1, i1)* @test_and_branch, !6}
!2 = !{i32 (i1, i1)* @test_or_select, !6}
!3 = !{i32 (i1, i1)* @test_or_branch, !6}
!4 = !{i32 (i1, i32, i32)* @test_and_select_negative, !6}
!6 = !{}
