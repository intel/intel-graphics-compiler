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
; %b = trunc i128 %a to i8
;
; into
; %be = bitcast <4 x float> %src to <16 x i8>
; %b = extractelement <16 x i8> %be, i32 0

define i8 @test_bitcast_pattern3(<4 x float> %src1) {
; CHECK-LABEL: define i8 @test_bitcast_pattern3(
; CHECK-SAME: <4 x float> [[SRC1:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast <4 x float> [[SRC1]] to <16 x i8>
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <16 x i8> [[TMP1]], i32 0
; CHECK-NEXT:    ret i8 [[TMP2]]
;
  %1 = bitcast <4 x float> %src1 to i128
  %2 = trunc i128 %1 to i8
  ret i8 %2
}

!igc.functions = !{!0}

!0 = !{i8 (<4 x float>)* @test_bitcast_pattern3, !2}
!2 = !{}
