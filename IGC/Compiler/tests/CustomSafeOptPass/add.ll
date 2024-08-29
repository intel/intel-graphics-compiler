;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s

; CustomSafeOpt will try to move addition with constant, from
; %1 = add i32 %a, 14
; %2 = add i32 %1, %b
; to
; %1 = add i32 %a, %b
; %2 = add i32 %1, 14

define i32 @test_customsafe_add(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add(
; CHECK:    [[TMP1:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add i32 %a, 14
  %2 = add i32 %1, %b
  ret i32 %2
}

; nuw is propagated only if set on both adds

define i32 @test_customsafe_add_nuw(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nuw(
; CHECK:    [[TMP1:%.*]] = add nuw i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add nuw i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add nuw i32 %a, 14
  %2 = add nuw i32 %1, %b
  ret i32 %2
}

define i32 @test_customsafe_add_no_nuw(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_no_nuw(
; CHECK:    [[TMP1:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add nuw i32 %a, 14
  %2 = add i32 %1, %b
  ret i32 %2
}

; Negative checks:
; transfromation is not applied if nsw is set

define i32 @test_customsafe_add_nsw_1(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_1(
; CHECK-NEXT:    [[TMP1:%.*]] = add nsw i32 [[A:%.*]], 14
; CHECK-NEXT:    [[TMP2:%.*]] = add i32 [[TMP1]], [[B:%.*]]
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %1 = add nsw i32 %a, 14
  %2 = add i32 %1, %b
  ret i32 %2
}

define i32 @test_customsafe_add_nsw_2(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_2(
; CHECK-NEXT:    [[TMP1:%.*]] = add i32 [[A:%.*]], 14
; CHECK-NEXT:    [[TMP2:%.*]] = add nsw i32 [[TMP1]], [[B:%.*]]
; CHECK-NEXT:    ret i32 [[TMP2]]
;
  %1 = add i32 %a, 14
  %2 = add nsw i32 %1, %b
  ret i32 %2
}
