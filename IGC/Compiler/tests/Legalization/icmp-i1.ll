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
; Legalization: icmp i1
; ------------------------------------------------

; Checks legalization of icmp with i1 type
; to icmp with i8 type

define i1 @test_icmp_u(i1 %a, i1 %b) {
; CHECK-LABEL: define i1 @test_icmp_u(
; CHECK-SAME: i1 [[A:%.*]], i1 [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = zext i1 [[A]] to i8
; CHECK:    [[TMP2:%.*]] = zext i1 [[B]] to i8
; CHECK:    [[TMP3:%.*]] = icmp ule i8 [[TMP1]], [[TMP2]]
; CHECK:    ret i1 [[TMP3]]
;
  %1 = icmp ule i1 %a, %b
  ret i1 %1
}

define i1 @test_icmp_s(i1 %a, i1 %b) {
; CHECK-LABEL: define i1 @test_icmp_s(
; CHECK-SAME: i1 [[A:%.*]], i1 [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sext i1 [[A]] to i8
; CHECK:    [[TMP2:%.*]] = sext i1 [[B]] to i8
; CHECK:    [[TMP3:%.*]] = icmp sle i8 [[TMP1]], [[TMP2]]
; CHECK:    ret i1 [[TMP3]]
;
  %1 = icmp sle i1 %a, %b
  ret i1 %1
}

!igc.functions = !{!0, !1}

!0 = !{i1 (i1, i1)* @test_icmp_u, !2}
!1 = !{i1 (i1, i1)* @test_icmp_s, !2}
!2 = !{}
