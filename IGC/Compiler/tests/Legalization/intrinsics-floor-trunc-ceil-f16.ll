;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformskl -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Checks legalization of floor, ceil and trunc Intrinsics
; for half type when it's not supported on platform

define half @test_floor(half %s1) {
; CHECK-LABEL: define half @test_floor(
; CHECK-SAME: half [[S1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = fpext half [[S1]] to float
; CHECK-NEXT:    [[TMP2:%.*]] = call float @llvm.floor.f32(float [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = fptrunc float [[TMP2]] to half
; CHECK-NEXT:    ret half [[TMP3]]
;
  %1 = call half @llvm.floor.f16(half %s1)
  ret half %1
}

define half @test_ceil(half %s1) {
; CHECK-LABEL: define half @test_ceil(
; CHECK-SAME: half [[S1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = fpext half [[S1]] to float
; CHECK-NEXT:    [[TMP2:%.*]] = call float @llvm.ceil.f32(float [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = fptrunc float [[TMP2]] to half
; CHECK-NEXT:    ret half [[TMP3]]
;
  %1 = call half @llvm.ceil.f16(half %s1)
  ret half %1
}

define half @test_trunc(half %s1) {
; CHECK-LABEL: define half @test_trunc(
; CHECK-SAME: half [[S1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = fpext half [[S1]] to float
; CHECK-NEXT:    [[TMP2:%.*]] = call float @llvm.trunc.f32(float [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = fptrunc float [[TMP2]] to half
; CHECK-NEXT:    ret half [[TMP3]]
;
  %1 = call half @llvm.trunc.f16(half %s1)
  ret half %1
}

declare half @llvm.floor.f16(half)
declare half @llvm.ceil.f16(half)
declare half @llvm.trunc.f16(half)

!igc.functions = !{!0, !1, !2}

!0 = !{half (half)* @test_floor, !3}
!1 = !{half (half)* @test_ceil, !3}
!2 = !{half (half)* @test_trunc, !3}
!3 = !{}
