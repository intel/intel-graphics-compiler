;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: store
; ------------------------------------------------

; Checks legalization of store constant vector types

define void @test_store_constdatavec(<4 x float>* %sptr) {
; CHECK-LABEL: define void @test_store_constdatavec(
; CHECK-SAME: <4 x float>* [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x float> undef, float 1.000000e+00, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x float> [[TMP1]], float 2.000000e+00, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x float> [[TMP2]], float 3.000000e+00, i32 2
; CHECK:    [[TMP4:%.*]] = insertelement <4 x float> [[TMP3]], float 4.000000e+00, i32 3
; CHECK:    store <4 x float> [[TMP4]], <4 x float>* [[SPTR]]
; CHECK:    ret void
;
  store <4 x float> <float 1.0, float 2.0, float 3.0, float 4.0>, <4 x float>* %sptr
  ret void
}

define void @test_store_constvec(<4 x float>* %sptr) {
; CHECK-LABEL: define void @test_store_constvec(
; CHECK-SAME: <4 x float>* [[SPTR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <4 x float> undef, float 1.000000e+00, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x float> [[TMP1]], float 1.000000e+00, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <4 x float> [[TMP2]], float 1.000000e+00, i32 2
; CHECK:    store <4 x float> [[TMP3]], <4 x float>* [[SPTR]]
; CHECK:    ret void
;
  store <4 x float> <float 1.0, float 1.0, float 1.0, float undef>, <4 x float>* %sptr
  ret void
}

define void @test_store_constaggrzero(<4 x i32>* %sptr) {
; CHECK-LABEL: define void @test_store_constaggrzero(
; CHECK-SAME: <4 x i32>* [[SPTR:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 0, i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 0, i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 0, i32 2
; CHECK-NEXT:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 0, i32 3
; CHECK-NEXT:    store <4 x i32> [[TMP4]], <4 x i32>* [[SPTR]], align 16
; CHECK-NEXT:    ret void
;
  store <4 x i32> zeroinitializer, <4 x i32>* %sptr
  ret void
}

!igc.functions = !{!0, !1, !2}
!0 = !{void (<4 x float>*)* @test_store_constdatavec, !3}
!1 = !{void (<4 x float>*)* @test_store_constvec, !3}
!3 = !{}
!2 = !{void (<4 x i32>*)* @test_store_constaggrzero, !3}
