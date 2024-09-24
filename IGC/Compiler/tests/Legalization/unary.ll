;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; due to bfloat ty
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: Unary instructions
; ------------------------------------------------
;
; Pass checks that unary operations have intermediate step(conversion to float)
; when operating on bfloat

define i32 @test_convert_to_int(bfloat %src1) {
; CHECK-LABEL: define i32 @test_convert_to_int(
; CHECK-SAME: bfloat [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fpext bfloat [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fptosi float [[TMP1]] to i32
; CHECK:    ret i32 [[TMP2]]
;
  %1 = fptosi bfloat %src1 to i32
  ret i32 %1
}

define i32 @test_convert_to_uint(bfloat %src1) {
; CHECK-LABEL: define i32 @test_convert_to_uint(
; CHECK-SAME: bfloat [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fpext bfloat [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fptoui float [[TMP1]] to i32
; CHECK:    ret i32 [[TMP2]]
;
  %1 = fptoui bfloat %src1 to i32
  ret i32 %1
}

define bfloat @test_convert_from_int(i32 %src1) {
; CHECK-LABEL: define bfloat @test_convert_from_int(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sitofp i32 [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fptrunc float [[TMP1]] to bfloat
; CHECK:    ret bfloat [[TMP2]]
;
  %1 = sitofp i32 %src1 to bfloat
  ret bfloat %1
}

define bfloat @test_convert_from_uint(i32 %src1) {
; CHECK-LABEL: define bfloat @test_convert_from_uint(
; CHECK-SAME: i32 [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = uitofp i32 [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fptrunc float [[TMP1]] to bfloat
; CHECK:    ret bfloat [[TMP2]]
;
  %1 = uitofp i32 %src1 to bfloat
  ret bfloat %1
}

define double @test_fpext_from_bf_to_double(bfloat %src1) {
; CHECK-LABEL: define double @test_fpext_from_bf_to_double(
; CHECK-SAME: bfloat [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fpext bfloat [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fpext float [[TMP1]] to double
; CHECK:    ret double [[TMP2]]
;
  %1 = fpext bfloat %src1 to double
  ret double %1
}

define bfloat @test_fptrunc_from_double_to_bf(double %src1) {
; CHECK-LABEL: define bfloat @test_fptrunc_from_double_to_bf(
; CHECK-SAME: double [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fptrunc double [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fptrunc float [[TMP1]] to bfloat
; CHECK:    ret bfloat [[TMP2]]
;
  %1 = fptrunc double %src1 to bfloat
  ret bfloat %1
}

define bfloat @test_fneg(bfloat %src1) {
; CHECK-LABEL: define bfloat @test_fneg(
; CHECK-SAME: bfloat [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fpext bfloat [[SRC1]] to float
; CHECK:    [[TMP2:%.*]] = fneg float [[TMP1]]
; CHECK:    [[TMP3:%.*]] = fptrunc float [[TMP2]] to bfloat
; CHECK:    ret bfloat [[TMP3]]
;
  %1 = fneg bfloat %src1
  ret bfloat %1
}

define <4 x bfloat> @test_vector(<4 x bfloat> %src1) {
; CHECK-LABEL: define <4 x bfloat> @test_vector(
; CHECK-SAME: <4 x bfloat> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fpext <4 x bfloat> [[SRC1]] to <4 x float>
; CHECK:    [[TMP2:%.*]] = fneg <4 x float> [[TMP1]]
; CHECK:    [[TMP3:%.*]] = fptrunc <4 x float> [[TMP2]] to <4 x bfloat>
; CHECK:    ret <4 x bfloat> [[TMP3]]
;
  %1 = fneg <4 x bfloat> %src1
  ret <4 x bfloat> %1
}

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7}
!0 = !{i32 (bfloat)* @test_convert_to_int, !8}
!1 = !{i32 (bfloat)* @test_convert_to_uint, !8}
!2 = !{bfloat (i32)* @test_convert_from_int, !8}
!3 = !{bfloat (i32)* @test_convert_from_uint, !8}
!4 = !{double (bfloat)* @test_fpext_from_bf_to_double, !8}
!5 = !{bfloat (double)* @test_fptrunc_from_double_to_bf, !8}
!6 = !{bfloat (bfloat)* @test_fneg, !8}
!7 = !{<4 x bfloat> (<4 x bfloat>)* @test_vector, !8}
!8 = !{}
