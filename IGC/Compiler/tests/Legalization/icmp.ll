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
; Legalization: icmp
; ------------------------------------------------

; Checks legalization of icmp with illegal int type
; produced by truncation to icmp with original type

define i1 @test_icmp_u(i32 %b) {
; CHECK-LABEL: define i1 @test_icmp_u(
; CHECK-SAME: i32 [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = and i32 [[B]], 16777215
; CHECK:    [[TMP2:%.*]] = icmp ule i32 [[TMP1]], 16777215
; CHECK:    ret i1 [[TMP2]]
;
  %1 = trunc i32 %b to i24
  %2 = icmp ule i24 %1, -1
  ret i1 %2
}

define i1 @test_icmp_s(i32 %b) {
; CHECK-LABEL: define i1 @test_icmp_s(
; CHECK-SAME: i32 [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i32 [[B]], 8
; CHECK:    [[TMP2:%.*]] = ashr i32 [[TMP1]], 8
; CHECK:    [[TMP3:%.*]] = icmp sle i32 [[TMP2]], -1
; CHECK:    ret i1 [[TMP3]]
;
  %1 = trunc i32 %b to i24
  %2 = icmp sle i24 %1, -1
  ret i1 %2
}

!igc.functions = !{!0, !1}

!0 = !{i1 (i32)* @test_icmp_u, !2}
!1 = !{i1 (i32)* @test_icmp_s, !2}
!2 = !{}
