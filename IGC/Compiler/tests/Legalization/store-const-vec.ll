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

define void @test_store_constaggrzero(ptr %sptr) {
; CHECK-LABEL: define void @test_store_constaggrzero(
; CHECK-SAME: ptr [[SPTR:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = insertelement <4 x i32> undef, i32 0, i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = insertelement <4 x i32> [[TMP1]], i32 0, i32 1
; CHECK-NEXT:    [[TMP3:%.*]] = insertelement <4 x i32> [[TMP2]], i32 0, i32 2
; CHECK-NEXT:    [[TMP4:%.*]] = insertelement <4 x i32> [[TMP3]], i32 0, i32 3
; CHECK-NEXT:    store <4 x i32> [[TMP4]], ptr [[SPTR]], align 16
; CHECK-NEXT:    ret void
;
  store <4 x i32> zeroinitializer, ptr %sptr, align 16
  ret void
}


!igc.functions = !{!0, !2, !3}

!0 = !{ptr @test_store_constdatavec, !1}
!1 = !{}
!2 = !{ptr @test_store_constvec, !1}
!3 = !{ptr @test_store_constaggrzero, !1}
