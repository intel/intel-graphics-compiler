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
; Legalization: bitcast illegal types after GVN
; ------------------------------------------------

; Test checks legalization for illegal bitcast src:
;
; bitcast i48 %src to <3 x i16>
; and
; bitcast i24 %src to <3 x i8>
;
; wheres %src is the result of trunc

define <3 x i16> @test_bitcast48(i64 %src1) {
; CHECK-LABEL: define <3 x i16> @test_bitcast48(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i64 [[SRC1]] to <4 x i16>
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <4 x i16> [[TMP1]], i32 0
; CHECK-NEXT:    [[TMP3:%.*]] = insertelement <3 x i16> undef, i16 [[TMP2]], i32 0
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <4 x i16> [[TMP1]], i32 1
; CHECK-NEXT:    [[TMP5:%.*]] = insertelement <3 x i16> [[TMP3]], i16 [[TMP4]], i32 1
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <4 x i16> [[TMP1]], i32 2
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <3 x i16> [[TMP5]], i16 [[TMP6]], i32 2
; CHECK-NEXT:    ret <3 x i16> [[TMP7]]
;
  %1 = trunc i64 %src1 to i48
  %2 = bitcast i48 %1 to <3 x i16>
  ret <3 x i16> %2
}

define <3 x i8> @test_bitcast24(i64 %src1) {
; CHECK-LABEL: define <3 x i8> @test_bitcast24(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i64 [[SRC1]] to <8 x i8>
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <8 x i8> [[TMP1]], i32 0
; CHECK-NEXT:    [[TMP3:%.*]] = insertelement <3 x i8> undef, i8 [[TMP2]], i32 0
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <8 x i8> [[TMP1]], i32 1
; CHECK-NEXT:    [[TMP5:%.*]] = insertelement <3 x i8> [[TMP3]], i8 [[TMP4]], i32 1
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <8 x i8> [[TMP1]], i32 2
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <3 x i8> [[TMP5]], i8 [[TMP6]], i32 2
; CHECK-NEXT:    ret <3 x i8> [[TMP7]]
;
  %1 = trunc i64 %src1 to i24
  %2 = bitcast i24 %1 to <3 x i8>
  ret <3 x i8> %2
}

; Not legalized
define <6 x i8> @test_bitcast48to6i8(i64 %src1) {
; CHECK-LABEL: define <6 x i8> @test_bitcast48to6i8(
; CHECK-SAME: i64 [[SRC1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i48
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast i48 [[TMP1]] to <6 x i8>
; CHECK-NEXT:    ret <6 x i8> [[TMP2]]
;
  %1 = trunc i64 %src1 to i48
  %2 = bitcast i48 %1 to <6 x i8>
  ret <6 x i8> %2
}


!igc.functions = !{!0, !1, !2}
!0 = !{<3 x i16> (i64)* @test_bitcast48, !3}
!1 = !{<3 x i8> (i64)* @test_bitcast24, !3}
!2 = !{<6 x i8> (i64)* @test_bitcast48to6i8, !3}
!3 = !{}
