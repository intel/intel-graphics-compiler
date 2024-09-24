;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; due to poison
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: shufflevector
; ------------------------------------------------

; Checks legalization of shufflevector to series of insert/extractelements
;

; whole new vector is created(could be optimized to reuse either s1 or s2)
define <4 x i32> @test_shuffle_samecount(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_samecount(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP2]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i32> [[TMP4]], i32 [[TMP5]], i32 2
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i32> [[TMP6]], i32 [[TMP7]], i32 3
; CHECK:    ret <4 x i32> [[TMP8]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 0, i32 1, i32 6, i32 7>
  ret <4 x i32> %1
}

; s1 value is reused
define <4 x i32> @test_shuffle_s1idx_match(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_s1idx_match(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[SRC1]], i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP2]], i32 [[TMP3]], i32 3
; CHECK:    ret <4 x i32> [[TMP4]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 7, i32 1, i32 2, i32 6>
  ret <4 x i32> %1
}

; s2 value is reused
define <4 x i32> @test_shuffle_s2idx_match(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_s2idx_match(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shufflevector <4 x i32> [[SRC1]], <4 x i32> [[SRC2]], <4 x i32> <i32 4, i32 5, i32 3, i32 1>
; CHECK:    ret <4 x i32> [[TMP1]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 4, i32 5, i32 3, i32 1>
  ret <4 x i32> %1
}

; s1 is returned
define <4 x i32> @test_shuffle_ret_s1(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_ret_s1(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    ret <4 x i32> [[SRC1]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  ret <4 x i32> %1
}

; s2 is returned
define <4 x i32> @test_shuffle_ret_s2(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_ret_s2(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    ret <4 x i32> [[SRC2]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 4, i32 5, i32 6, i32 7>
  ret <4 x i32> %1
}

;
; mask size != src size cases:
;
; greater
define <6 x i32> @test_shuffle_greater(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <6 x i32> @test_shuffle_greater(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC2]], i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <6 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP4:%.*]] = insertelement <6 x i32> [[TMP2]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP6:%.*]] = insertelement <6 x i32> [[TMP4]], i32 [[TMP5]], i32 2
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP8:%.*]] = insertelement <6 x i32> [[TMP6]], i32 [[TMP7]], i32 3
; CHECK:    [[TMP9:%.*]] = extractelement <4 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP10:%.*]] = insertelement <6 x i32> [[TMP8]], i32 [[TMP9]], i32 4
; CHECK:    [[TMP11:%.*]] = extractelement <4 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP12:%.*]] = insertelement <6 x i32> [[TMP10]], i32 [[TMP11]], i32 5
; CHECK:    ret <6 x i32> [[TMP12]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <6 x i32> <i32 4, i32 5, i32 6, i32 7, i32 1, i32 5>
  ret <6 x i32> %1
}

; less
define <3 x i32> @test_shuffle_less(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <3 x i32> @test_shuffle_less(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC2]], i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <3 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP4:%.*]] = insertelement <3 x i32> [[TMP2]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP6:%.*]] = insertelement <3 x i32> [[TMP4]], i32 [[TMP5]], i32 2
; CHECK:    ret <3 x i32> [[TMP6]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <3 x i32> <i32 4, i32 5, i32 1>
  ret <3 x i32> %1
}

; 1 element
define <1 x i32> @test_shuffle_1el(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <1 x i32> @test_shuffle_1el(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP2:%.*]] = insertelement <1 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    ret <1 x i32> [[TMP2]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <1 x i32> <i32 7>
  ret <1 x i32> %1
}

; Other cases:

; constant
define <8 x i32> @test_shuffle_const() {
; CHECK-LABEL: define <8 x i32> @test_shuffle_const() {
; CHECK:    [[TMP1:%.*]] = insertelement <8 x i32> undef, i32 0, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> [[TMP1]], i32 1, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 2, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <8 x i32> [[TMP3]], i32 3, i32 3
; CHECK:    [[TMP5:%.*]] = insertelement <8 x i32> [[TMP4]], i32 4, i32 4
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i32> [[TMP5]], i32 5, i32 5
; CHECK:    [[TMP7:%.*]] = insertelement <8 x i32> [[TMP6]], i32 7, i32 6
; CHECK:    [[TMP8:%.*]] = insertelement <8 x i32> [[TMP7]], i32 6, i32 7
; CHECK:    ret <8 x i32> [[TMP8]]
;
  %1 = shufflevector <4 x i32> <i32 1, i32 3, i32 5, i32 7>, <4 x i32> <i32 2, i32 4, i32 6, i32 0>, <8 x i32> <i32 7, i32 0, i32 4, i32 1, i32 5, i32 2, i32 3, i32 6>
  ret <8 x i32> %1
}

; poison/undef
define <4 x i32> @test_shuffle_undef(<4 x i32> %src1) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_undef(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> [[SRC1]], i32 1, i32 2
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 0, i32 3
; CHECK:    ret <4 x i32> [[TMP2]]
;
  %1 = shufflevector <4 x i32> %src1, <4 x i32> <i32 0, i32 1, i32 poison, i32 undef>, <4 x i32> <i32 0, i32 1, i32 5,i32 4>
  ret <4 x i32> %1
}

; vector produced by insertelement
define <4 x i32> @test_shuffle_insert_el(<4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_shuffle_insert_el(
; CHECK-SAME: <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> [[SRC2]], i32 13, i32 2
; CHECK:    ret <4 x i32> [[TMP1]]
;
  %1 = insertelement <4 x i32> %src1, i32 13, i32 0
  %2 = shufflevector <4 x i32> %1, <4 x i32> %src2, <4 x i32> <i32 4, i32 5, i32 0, i32 7>
  ret <4 x i32> %2
}


!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9}

!0 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_samecount, !10}
!1 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_s1idx_match, !10}
!2 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_ret_s1, !10}
!3 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_ret_s2, !10}
!4 = !{<6 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_greater, !10}
!5 = !{<3 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_less, !10}
!6 = !{<1 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_1el, !10}
!7 = !{<8 x i32> ()* @test_shuffle_const, !10}
!8 = !{<4 x i32> (<4 x i32>)* @test_shuffle_undef, !10}
!9 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_shuffle_insert_el, !10}
!10 = !{}
