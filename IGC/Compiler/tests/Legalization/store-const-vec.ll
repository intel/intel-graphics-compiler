;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -opaque-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: store
; ------------------------------------------------

; Checks legalization of store constant vector types

define void @test_store_constdatavec(ptr %sptr) {
; CHECK-LABEL: define void @test_store_constdatavec(
; CHECK-SAME: ptr [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x float> undef, float 1.000000e+00, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x float> [[TMP1]], float 2.000000e+00, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x float> [[TMP2]], float 3.000000e+00, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x float> [[TMP3]], float 4.000000e+00, i32 3
; CHECK:    store <4 x float> [[TMP4]], ptr [[SPTR]], align 16
; CHECK:    ret void
;
  store <4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>, ptr %sptr, align 16
  ret void
}

define void @test_store_constvec(ptr %sptr) {
; CHECK-LABEL: define void @test_store_constvec(
; CHECK-SAME: ptr [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x float> undef, float 1.000000e+00, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x float> [[TMP1]], float 1.000000e+00, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x float> [[TMP2]], float 1.000000e+00, i32 2
; CHECK:    store <4 x float> [[TMP3]], ptr [[SPTR]], align 16
; CHECK:    ret void
;
  store <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float undef>, ptr %sptr, align 16
  ret void
}

!igc.functions = !{!0, !2}

!0 = !{ptr @test_store_constdatavec, !1}
!1 = !{}
!2 = !{ptr @test_store_constvec, !1}
