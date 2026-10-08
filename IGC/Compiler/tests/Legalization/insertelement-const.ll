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
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 3, i32 2
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 4, i32 3
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 [[SRC]], i32 1
; CHECK:    ret <4 x i32> [[TMP4]]
;
  %1 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %src, i32 1
  ret <4 x i32> %1
}

define <4 x i32> @test_insertelem_constvec(i32 %src) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_constvec(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 -1, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 -2, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 [[SRC]], i32 2
; CHECK:    ret <4 x i32> [[TMP3]]
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
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 [[SRC]], i32 3
; CHECK:    ret <4 x i32> [[TMP4]]
;
  %1 = insertelement <4 x i32> zeroinitializer, i32 %src, i32 3
  ret <4 x i32> %1
}

; Constant lanes that a single-use insertelement chain overwrites are not
; materialized; a fully overwritten base becomes undef.

define <4 x i8> @test_insertelem_chain_all_lanes(<8 x i8> %src) {
; CHECK-LABEL: define <4 x i8> @test_insertelem_chain_all_lanes(
; CHECK-SAME: <8 x i8> [[SRC:%.*]]) {
; CHECK-NOT:  insertelement <4 x i8> {{.*}}, i8 0, i32
; CHECK:    [[E0:%.*]] = extractelement <8 x i8> [[SRC]], i32 0
; CHECK:    [[V0:%.*]] = insertelement <4 x i8> undef, i8 [[E0]], i64 0
; CHECK:    [[V1:%.*]] = insertelement <4 x i8> [[V0]], i8 {{%.*}}, i64 1
; CHECK:    [[V2:%.*]] = insertelement <4 x i8> [[V1]], i8 {{%.*}}, i64 2
; CHECK:    [[V3:%.*]] = insertelement <4 x i8> [[V2]], i8 {{%.*}}, i64 3
; CHECK:    ret <4 x i8> [[V3]]
;
  %e0 = extractelement <8 x i8> %src, i32 0
  %v0 = insertelement <4 x i8> <i8 poison, i8 poison, i8 0, i8 0>, i8 %e0, i64 0
  %e1 = extractelement <8 x i8> %src, i32 1
  %v1 = insertelement <4 x i8> %v0, i8 %e1, i64 1
  %e2 = extractelement <8 x i8> %src, i32 2
  %v2 = insertelement <4 x i8> %v1, i8 %e2, i64 2
  %e3 = extractelement <8 x i8> %src, i32 3
  %v3 = insertelement <4 x i8> %v2, i8 %e3, i64 3
  ret <4 x i8> %v3
}

; %v1 has a second use, so lanes 2 and 3 of the base are observable and stay.

define <4 x i32> @test_insertelem_chain_multi_use(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_chain_multi_use(
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 0, i32 2
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 0, i32 3
; CHECK:    [[V0:%.*]] = insertelement <4 x i32> [[TMP2]], i32 %a, i32 0
; CHECK:    [[V1:%.*]] = insertelement <4 x i32> [[V0]], i32 %b, i32 1
; CHECK:    [[V2:%.*]] = insertelement <4 x i32> [[V1]], i32 %c, i32 2
; CHECK:    [[V3:%.*]] = insertelement <4 x i32> [[V2]], i32 %d, i32 3
; CHECK:    [[SUM:%.*]] = add <4 x i32> [[V1]], [[V3]]
; CHECK:    ret <4 x i32> [[SUM]]
;
  %v0 = insertelement <4 x i32> zeroinitializer, i32 %a, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %b, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %c, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %d, i32 3
  %sum = add <4 x i32> %v1, %v3
  ret <4 x i32> %sum
}

; A variable index stops the chain walk, so all constant lanes stay.

define <4 x i32> @test_insertelem_var_index(i32 %src, i32 %idx) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_var_index(
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 1, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 2, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 3, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 4, i32 3
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i32> [[TMP4]], i32 %src, i32 %idx
; CHECK:    ret <4 x i32> [[TMP5]]
;
  %1 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %src, i32 %idx
  ret <4 x i32> %1
}

; An index wider than 64 bits stops the chain walk, so all constant lanes stay.

define <4 x i32> @test_insertelem_wide_index(i32 %src) {
; CHECK-LABEL: define <4 x i32> @test_insertelem_wide_index(
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 1, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 2, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 3, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 4, i32 3
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i32> [[TMP4]], i32 %src, i128 18446744073709551616
; CHECK:    ret <4 x i32> [[TMP5]]
;
  %1 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %src, i128 18446744073709551616
  ret <4 x i32> %1
}

!igc.functions = !{!0, !1, !2, !4, !5, !6, !7}

!0 = !{<4 x i32>(i32)* @test_insertelem_constdatavec, !3}
!1 = !{<4 x i32>(i32)* @test_insertelem_constvec, !3}
!2 = !{<4 x i32>(i32)* @test_insertelem_constaggzero, !3}
!3 = !{}
!4 = !{<4 x i8>(<8 x i8>)* @test_insertelem_chain_all_lanes, !3}
!5 = !{<4 x i32>(i32, i32, i32, i32)* @test_insertelem_chain_multi_use, !3}
!6 = !{<4 x i32>(i32, i32)* @test_insertelem_var_index, !3}
!7 = !{<4 x i32>(i32)* @test_insertelem_wide_index, !3}
