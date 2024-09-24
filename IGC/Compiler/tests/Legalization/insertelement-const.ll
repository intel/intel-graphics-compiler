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
; Legalization: insertelement
; ------------------------------------------------

; Checks legalization of insertelement with constant vectors

define <4 x i32> @test_insertelem_constdatavec(i32 %src) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_constdatavec(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 1, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 2, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 3, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 4, i32 3
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i32> [[TMP4]], i32 [[SRC]], i32 1
; CHECK:    ret <4 x i32> [[TMP5]]
;
  %1 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %src, i32 1
  ret <4 x i32> %1
}

define <4 x i32> @test_insertelem_constvec(i32 %src) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_constvec(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 -1, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 -2, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 -3, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 [[SRC]], i32 2
; CHECK:    ret <4 x i32> [[TMP4]]
;
  %1 = insertelement <4 x i32> <i32 -1, i32 -2, i32 -3, i32 undef>, i32 %src, i32 2
  ret <4 x i32> %1
}

define <4 x i32> @test_insertelem_constaggzero(i32 %src) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_constaggzero(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 0, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 0, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 0, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 0, i32 3
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i32> [[TMP4]], i32 [[SRC]], i32 3
; CHECK:    ret <4 x i32> [[TMP5]]
;
  %1 = insertelement <4 x i32> zeroinitializer, i32 %src, i32 3
  ret <4 x i32> %1
}

!igc.functions = !{!0, !1, !2}

!0 = !{<4 x i32>(i32)* @test_insertelem_constdatavec, !3}
!1 = !{<4 x i32>(i32)* @test_insertelem_constvec, !3}
!2 = !{<4 x i32>(i32)* @test_insertelem_constaggzero, !3}
!3 = !{}
