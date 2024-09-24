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

; Checks legalization of insertelement with data type i1
; to insertelement of i32 vector

define i1 @test_insertelem_i1(<8 x i1> %src1, i1 %src2, i32 %src3) {
; CHECK-LABEL: define i1 @test_insertelem_i1(
; CHECK-SAME: <8 x i1> [[SRC1:%.*]], i1 [[SRC2:%.*]], i32 [[SRC3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sext i1 [[SRC2]] to i32
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> undef, i32 [[TMP1]], i32 7
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 -1, i32 1
; CHECK:    [[TMP4:%.*]] = extractelement <8 x i32> [[TMP3]], i32 [[SRC3]]
; CHECK:    [[TMP5:%.*]] = trunc i32 [[TMP4]] to i1
; CHECK:    ret i1 [[TMP5]]
;
  %1 = insertelement <8 x i1> %src1, i1 %src2, i32 7
  %2 = insertelement <8 x i1> %1, i1 true, i32 1
  %3 = extractelement <8 x i1> %2, i32 %src3
  ret i1 %3
}

define i32 @test_insertelem_i1_sext(<8 x i1> %src1, i1 %src2, i32 %src3) {
; CHECK-LABEL: define i32 @test_insertelem_i1_sext(
; CHECK-SAME: <8 x i1> [[SRC1:%.*]], i1 [[SRC2:%.*]], i32 [[SRC3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sext i1 [[SRC2]] to i32
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> undef, i32 [[TMP1]], i32 7
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 -1, i32 1
; CHECK:    [[TMP4:%.*]] = extractelement <8 x i32> [[TMP3]], i32 [[SRC3]]
; CHECK:    ret i32 [[TMP4]]
;
  %1 = insertelement <8 x i1> %src1, i1 %src2, i32 7
  %2 = insertelement <8 x i1> %1, i1 true, i32 1
  %3 = extractelement <8 x i1> %2, i32 %src3
  %4 = sext i1 %3 to i32
  ret i32 %4
}

!igc.functions = !{!0, !1}

!0 = !{i1 (<8 x i1>, i1, i32)* @test_insertelem_i1, !2}
!1 = !{i32 (<8 x i1>, i1, i32)* @test_insertelem_i1_sext, !2}
!2 = !{}
