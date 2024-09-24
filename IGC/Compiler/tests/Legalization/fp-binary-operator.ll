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
; Legalization: Call FPBinaryOperator intrinsic
; ------------------------------------------------
;
; Pass checks that
; FPBinaryOperator instrinsic call is legalized back to
; llvm fp binary operations

; Check that this is done for:
; FAdd,       0x1
; FSub,       0x2
; FMul,       0x4
; FDiv,       0x8

define float @test_fadd(float %src1, float %src2) {
; CHECK-LABEL: define float @test_fadd(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fadd float [[SRC1]], [[SRC2]]
; CHECK:    ret float [[TMP1]]
;
  %1 = call float @llvm.genx.GenISA.FPBinaryOperator.f32(float %src1, float %src2, i32 1)
  ret float %1
}

define float @test_fsub(float %src1, float %src2) {
; CHECK-LABEL: define float @test_fsub(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fsub float [[SRC1]], [[SRC2]]
; CHECK:    ret float [[TMP1]]
;
  %1 = call float @llvm.genx.GenISA.FPBinaryOperator.f32(float %src1, float %src2, i32 2)
  ret float %1
}

define float @test_fmul(float %src1, float %src2) {
; CHECK-LABEL: define float @test_fmul(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fmul float [[SRC1]], [[SRC2]]
; CHECK:    ret float [[TMP1]]
;
  %1 = call float @llvm.genx.GenISA.FPBinaryOperator.f32(float %src1, float %src2, i32 4)
  ret float %1
}

define float @test_fdiv(float %src1, float %src2) {
; CHECK-LABEL: define float @test_fdiv(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fdiv float [[SRC1]], [[SRC2]]
; CHECK:    ret float [[TMP1]]
;
  %1 = call float @llvm.genx.GenISA.FPBinaryOperator.f32(float %src1, float %src2, i32 8)
  ret float %1
}

; Check for other types

define half @test_fadd_half(half %src1, half %src2) {
; CHECK-LABEL: define half @test_fadd_half(
; CHECK-SAME: half [[SRC1:%.*]], half [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fadd half [[SRC1]], [[SRC2]]
; CHECK:    ret half [[TMP1]]
;
  %1 = call half @llvm.genx.GenISA.FPBinaryOperator.f16(half %src1, half %src2, i32 1)
  ret half %1
}

define bfloat @test_fadd_bfloat(bfloat %src1, bfloat %src2) {
; CHECK-LABEL: define bfloat @test_fadd_bfloat(
; CHECK-SAME: bfloat [[SRC1:%.*]], bfloat [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fadd bfloat [[SRC1]], [[SRC2]]
; CHECK:    ret bfloat [[TMP1]]
;
  %1 = call bfloat @llvm.genx.GenISA.FPBinaryOperator.bf16(bfloat %src1, bfloat %src2, i32 1)
  ret bfloat %1
}

define double @test_fadd_double(double %src1, double %src2) {
; CHECK-LABEL: define double @test_fadd_double(
; CHECK-SAME: double [[SRC1:%.*]], double [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fadd double [[SRC1]], [[SRC2]]
; CHECK:    ret double [[TMP1]]
;
  %1 = call double @llvm.genx.GenISA.FPBinaryOperator.f64(double %src1, double %src2, i32 1)
  ret double %1
}

; Check that flags are copied

define float @test_fadd_flags(float %src1, float %src2) {
; CHECK-LABEL: define float @test_fadd_flags(
; CHECK-SAME: float [[SRC1:%.*]], float [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = fadd nsz float [[SRC1]], [[SRC2]]
; CHECK:    ret float [[TMP1]]
;
  %1 = call nsz float @llvm.genx.GenISA.FPBinaryOperator.f32(float %src1, float %src2, i32 1)
  ret float %1
}

declare float @llvm.genx.GenISA.FPBinaryOperator.f32(float, float, i32)
declare half @llvm.genx.GenISA.FPBinaryOperator.f16(half, half, i32)
declare bfloat @llvm.genx.GenISA.FPBinaryOperator.bf16(bfloat, bfloat, i32)
declare double @llvm.genx.GenISA.FPBinaryOperator.f64(double, double, i32)

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7}
!0 = !{float (float, float)* @test_fadd, !8}
!1 = !{float (float, float)* @test_fsub, !8}
!2 = !{float (float, float)* @test_fmul, !8}
!3 = !{float (float, float)* @test_fdiv, !8}
!4 = !{half (half, half)* @test_fadd_half, !8}
!5 = !{bfloat (bfloat, bfloat)* @test_fadd_bfloat, !8}
!6 = !{double (double, double)* @test_fadd_double, !8}
!7 = !{float (float, float)* @test_fadd_flags, !8}
!8 = !{}
