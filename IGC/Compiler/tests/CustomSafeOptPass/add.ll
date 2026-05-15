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

; With the NSW-preservation bailout removed, the reassociation also fires
; when nsw is set asymmetrically on the two adds. The resulting adds are
; emitted without nsw (the inner i32 const-add becomes dead code).

define i32 @test_customsafe_add_nsw_1(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_1(
; CHECK:    [[TMP1:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add nsw i32 %a, 14
  %2 = add i32 %1, %b
  ret i32 %2
}

define i32 @test_customsafe_add_nsw_2(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_2(
; CHECK:    [[TMP1:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add i32 %a, 14
  %2 = add nsw i32 %1, %b
  ret i32 %2
}

; nsw and nuw propagate when both flags are set on both adds.

define i32 @test_customsafe_add_nsw_nuw_both(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_nuw_both(
; CHECK:    [[TMP1:%.*]] = add nuw nsw i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add nuw nsw i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add nuw nsw i32 %a, 14
  %2 = add nuw nsw i32 %1, %b
  ret i32 %2
}

; asymmetric flags: reassociation still applies; output adds carry no flags.

define i32 @test_customsafe_add_nsw_nuw_asymmetric(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_add_nsw_nuw_asymmetric(
; CHECK:    [[TMP1:%.*]] = add i32 [[A:%.*]], [[B:%.*]]
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 14
; CHECK:    ret i32 [[TMP2]]
;
  %1 = add nuw nsw i32 %a, 14
  %2 = add nsw i32 %1, %b
  ret i32 %2
}
