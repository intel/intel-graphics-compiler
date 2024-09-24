;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformdg2 -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: select double
; ------------------------------------------------

; Checks legalization of select with double type(when it's not supported)
; into two selects of i32
;

define double @test_select(i1 %cc, double %src1, double %src2) {
; CHECK-LABEL: define double @test_select(
; CHECK-SAME: i1 [[CC:%.*]], double [[SRC1:%.*]], double [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast double [[SRC1]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast double [[SRC2]] to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = select i1 [[CC]], i32 [[TMP2]], i32 [[TMP5]]
; CHECK:    [[TMP8:%.*]] = select i1 [[CC]], i32 [[TMP3]], i32 [[TMP6]]
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP8]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to double
; CHECK:    ret double [[TMP11]]
;
  %1 = select i1 %cc, double %src1, double %src2
  ret double %1
}

!igc.functions = !{!0}

!0 = !{double (i1, double, double)* @test_select, !1}
!1 = !{}
