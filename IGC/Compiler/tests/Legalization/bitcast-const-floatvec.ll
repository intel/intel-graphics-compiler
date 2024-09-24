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
; Legalization: bitcast float vector to double
; ------------------------------------------------

; Test checks legalization for constant <2 x float> bitcast to double

define double @test_bitcast() {
; CHECK-LABEL: define double @test_bitcast() {
; CHECK:    ret double 0x7FF800007FF80000
;
  %1 = bitcast <2 x float> <float 0x7FFF000000000000, float 0x7FFF000000000000> to double
  ret double %1                                                                                                                                                                              }

define double @test_bitcast_not(<2 x float> %src1) {
; CHECK-LABEL: define double @test_bitcast_not(
; CHECK-SAME: <2 x float> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = insertelement <2 x float> undef, float 0x7FFF000000000000, i32 0
; CHECK:    [[TMP2:%.*]] = insertelement <2 x float> [[TMP1]], float 0x7FFF000000000000, i32 1
; CHECK:    [[TMP3:%.*]] = insertelement <2 x float> [[TMP2]], float 1.000000e+00, i32 0
; CHECK:    [[TMP4:%.*]] = insertelement <2 x float> <float 0x7FFF000000000000, float 0x7FFF000000000000>, float 1.000000e+00, i32 0
; CHECK:    [[TMP5:%.*]] = bitcast <2 x float> [[TMP3]] to double
; CHECK:    ret double [[TMP5]]
;
  %1 = insertelement <2 x float> <float 0x7FFF000000000000, float 0x7FFF000000000000>, float 1.0, i32 0
  %2 = bitcast <2 x float> %1 to double
  ret double %2
}

!igc.functions = !{!0, !1}

!0 = !{double ()* @test_bitcast, !2}
!1 = !{double (<2 x float>)* @test_bitcast_not, !2}
!2 = !{}
