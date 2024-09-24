;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: bitcast to illegal type
; ------------------------------------------------

; Checks legalization of this pattern:
;
; %a = bitcast <4 x float> %src to i128
; %b = trunc i128 %a to i96
; %c = bitcast i96 %b to <3 x i32>
; %e1 = extractelement <3 x i32> %c, i32 0
; %e2 = extractelement <3 x i32> %c, i32 1
;
; into
; %e1f = extractelement <4 x float> %src1, i32 0
; %e1 = bitcast float %e1f to i32
; %e2f = extractelement <4 x float> %src1, i32 1
; %e2 = bitcast float %e2f to i32

define i32 @test_bitcast_pattern2(<4 x float> %src1) {
; CHECK-LABEL: define i32 @test_bitcast_pattern2(
; CHECK-SAME: <4 x float> [[SRC1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = bitcast float [[TMP1]] to i32
; CHECK:    [[TMP3:%.*]] = extractelement <4 x float> [[SRC1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast float [[TMP3]] to i32
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP2]], [[TMP4]]
; CHECK:    ret i32 [[TMP5]]
;
  %1 = bitcast <4 x float> %src1 to i128
  %2 = trunc i128 %1 to i96
  %3 = bitcast i96 %2 to <3 x i32>
  %4 = extractelement <3 x i32> %3, i32 0
  %5 = extractelement <3 x i32> %3, i32 1
  %6 = add i32 %4, %5
  ret i32 %6
}

define i32 @test_bitcast_pattern2_idx(<4 x float> %src1, i32 %src2) {
; CHECK-LABEL: define i32 @test_bitcast_pattern2_idx(
; CHECK-SAME: <4 x float> [[SRC1:%.*]], i32 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> [[SRC1]], i32 [[SRC2]]
; CHECK:    [[TMP2:%.*]] = bitcast float [[TMP1]] to i32
; CHECK:    ret i32 [[TMP2]]
;
  %1 = bitcast <4 x float> %src1 to i128
  %2 = trunc i128 %1 to i96
  %3 = bitcast i96 %2 to <3 x i32>
  %4 = extractelement <3 x i32> %3, i32 %src2
  ret i32 %4
}

!igc.functions = !{!0, !1}

!0 = !{i32 (<4 x float>)* @test_bitcast_pattern2, !2}
!1 = !{i32 (<4 x float>, i32)* @test_bitcast_pattern2_idx, !2}
!2 = !{}
