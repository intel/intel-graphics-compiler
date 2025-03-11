;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -verify -S %s -o - | FileCheck %s

; ------------------------------------------------
; Legalization: bitcast illegal types after GVN
; ------------------------------------------------

define i16 @test_bitcast_pattern2(<4 x float> %src1, i32 %src2) {
; CHECK-LABEL: define i16 @test_bitcast_pattern2(
; CHECK-SAME: <4 x float> [[SRC1:%.*]], i32 [[SRC2:%.*]]) {
; CHECK: [[TMP2:%.*]] = bitcast <4 x float> [[SRC1]] to <2 x i64>
; CHECK-NEXT: [[TMP3:%.*]] = extractelement <2 x i64> [[TMP2]], i32 0
; CHECK-NEXT: [[TMP4:%.*]] = bitcast i64 [[TMP3]] to <4 x i16>
; CHECK-NEXT: [[TMP5:%.*]] = extractelement <4 x i16> [[TMP4]], i32 1
; CHECK-NEXT: ret i16 [[TMP5]]
;
  %1 = bitcast <4 x float> %src1 to i128
  %2 = trunc i128 %1 to i64
  %3 = bitcast i64 %2 to <4 x i16>
  %4 = extractelement <4 x i16> %3, i32 1
  ret i16 %4
}

!igc.functions = !{!0}

!0 = !{i16 (<4 x float>, i32)* @test_bitcast_pattern2, !1}
!1 = !{}