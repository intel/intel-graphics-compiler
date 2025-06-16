;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce -preserve-nan < %s | FileCheck %s
; ------------------------------------------------
; Legalization: fcmp unordered
; ------------------------------------------------

; Checks legalization of fcmp for nan handling
; since visa has limited support for unordered predicates (and no one)

; ueq: yields true if either operand is a QNAN or op1 is equal to op2.
define i1 @test_fcmp_ueq(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ueq(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK-DAG:    [[TMP1:%.*]] = fcmp une float [[A]], [[A]]
; CHECK-DAG:    [[TMP2:%.*]] = fcmp une float [[B]], [[B]]
; CHECK:        [[TMP3:%.*]] = or i1 [[TMP1]], [[TMP2]]
; CHECK:        [[TMP4:%.*]] = fcmp oeq float [[A]], [[B]]
; CHECK:        [[TMP5:%.*]] = or i1 [[TMP3]], [[TMP4]]
; CHECK:        ret i1 [[TMP5]]
;
  %1 = fcmp ueq float %a, %b
  ret i1 %1
}

; ugt: yields true if either operand is a QNAN or op1 is greater than op2.
define i1 @test_fcmp_ugt(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ugt(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp ole float [[A]], [[B]]
; CHECK:    [[TMP2:%.*]] = xor i1 [[TMP1]], true
; CHECK:    ret i1 [[TMP2]]
;
  %1 = fcmp ugt float %a, %b
  ret i1 %1
}

; uge: yields true if either operand is a QNAN or op1 is greater than or equal to op2.
define i1 @test_fcmp_uge(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_uge(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp olt float [[A]], [[B]]
; CHECK:    [[TMP2:%.*]] = xor i1 [[TMP1]], true
; CHECK:    ret i1 [[TMP2]]
;
  %1 = fcmp uge float %a, %b
  ret i1 %1
}

; ult: yields true if either operand is a QNAN or op1 is less than op2.
define i1 @test_fcmp_ult(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ult(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp oge float [[A]], [[B]]
; CHECK:    [[TMP2:%.*]] = xor i1 [[TMP1]], true
; CHECK:    ret i1 [[TMP2]]
;
  %1 = fcmp ult float %a, %b
  ret i1 %1
}

; ule: yields true if either operand is a QNAN or op1 is less than or equal to op2.
define i1 @test_fcmp_ule(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ule(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp ogt float [[A]], [[B]]
; CHECK:    [[TMP2:%.*]] = xor i1 [[TMP1]], true
; CHECK:    ret i1 [[TMP2]]
;
  %1 = fcmp ule float %a, %b
  ret i1 %1
}

; uno: yields true if either operand is a QNAN.
define i1 @test_fcmp_uno(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_uno(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp une float [[A]], [[A]]
; CHECK:    [[TMP2:%.*]] = fcmp une float [[B]], [[B]]
; CHECK:    [[TMP3:%.*]] = or i1 [[TMP1]], [[TMP2]]
; CHECK:    ret i1 [[TMP3]]
;
  %1 = fcmp uno float %a, %b
  ret i1 %1
}

; one: yields true if both operands are not a QNAN and op1 is not equal to op2.
define i1 @test_fcmp_one(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_one(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK-DAG:    [[TMP1:%.*]] = fcmp oeq float [[B]], [[B]]
; CHECK-DAG:    [[TMP2:%.*]] = fcmp oeq float [[A]], [[A]]
; CHECK:        [[TMP3:%.*]] = and i1 [[TMP2]], [[TMP1]]
; CHECK:        [[TMP4:%.*]] = fcmp une float [[A]], [[B]]
; CHECK:        [[TMP5:%.*]] = and i1 [[TMP3]], [[TMP4]]
; CHECK:        ret i1 [[TMP5]]
;
  %1 = fcmp one float %a, %b
  ret i1 %1
}

; ord: yields true if both operands are not a QNAN.
define i1 @test_fcmp_ord(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ord(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK-DAG:    [[TMP1:%.*]] = fcmp oeq float [[B]], [[B]]
; CHECK-DAG:    [[TMP2:%.*]] = fcmp oeq float [[A]], [[A]]
; CHECK:        [[TMP3:%.*]] = and i1 [[TMP2]], [[TMP1]]
; CHECK:        ret i1 [[TMP3]]
;
  %1 = fcmp ord float %a, %b
  ret i1 %1
}

; ================================================
; Negative checks, below predicates are not changed:
; ================================================

; une: yields true if either operand is a QNAN or op1 is not equal to op2.
define i1 @test_fcmp_une(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_une(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp une float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp une float %a, %b
  ret i1 %1
}

; true: always yields true, regardless of operands.
define i1 @test_fcmp_true(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_true(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp true float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp true float %a, %b
  ret i1 %1
}

; false: always yields false, regardless of operands.
define i1 @test_fcmp_false(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_false(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp false float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp false float %a, %b
  ret i1 %1
}

; oeq: yields true if both operands are not a QNAN and op1 is equal to op2.
define i1 @test_fcmp_oeq(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_oeq(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp oeq float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp oeq float %a, %b
  ret i1 %1
}

; ogt: yields true if both operands are not a QNAN and op1 is greater than op2.
define i1 @test_fcmp_ogt(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ogt(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp ogt float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp ogt float %a, %b
  ret i1 %1
}

; oge: yields true if both operands are not a QNAN and op1 is greater than or equal to op2.
define i1 @test_fcmp_oge(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_oge(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp oge float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp oge float %a, %b
  ret i1 %1
}

; olt: yields true if both operands are not a QNAN and op1 is less than op2.
define i1 @test_fcmp_olt(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_olt(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp olt float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp olt float %a, %b
  ret i1 %1
}

; ole: yields true if both operands are not a QNAN and op1 is less than or equal to op2
define i1 @test_fcmp_ole(float %a, float %b) {
; CHECK-LABEL: define i1 @test_fcmp_ole(
; CHECK-SAME: float [[A:%.*]], float [[B:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fcmp ole float [[A]], [[B]]
; CHECK:    ret i1 [[TMP1]]
;
  %1 = fcmp ole float %a, %b
  ret i1 %1
}

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}
!0 = !{i1 (float, float)* @test_fcmp_ueq, !16}
!1 = !{i1 (float, float)* @test_fcmp_ugt, !16}
!2 = !{i1 (float, float)* @test_fcmp_uge, !16}
!3 = !{i1 (float, float)* @test_fcmp_ult, !16}
!4 = !{i1 (float, float)* @test_fcmp_ule, !16}
!5 = !{i1 (float, float)* @test_fcmp_une, !16}
!6 = !{i1 (float, float)* @test_fcmp_uno, !16}
!7 = !{i1 (float, float)* @test_fcmp_true, !16}
!8 = !{i1 (float, float)* @test_fcmp_false, !16}
!9 = !{i1 (float, float)* @test_fcmp_one, !16}
!10 = !{i1 (float, float)* @test_fcmp_ord, !16}
!11 = !{i1 (float, float)* @test_fcmp_oeq, !16}
!12 = !{i1 (float, float)* @test_fcmp_ogt, !16}
!13 = !{i1 (float, float)* @test_fcmp_oge, !16}
!14 = !{i1 (float, float)* @test_fcmp_olt, !16}
!15 = !{i1 (float, float)* @test_fcmp_ole, !16}
!16 = !{}
