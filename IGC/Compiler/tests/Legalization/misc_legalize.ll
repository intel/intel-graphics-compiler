;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-int-type-legalizer -S < %s | FileCheck %s
;
; Test checks misс promotions in lagalization

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <2 x i32> @test_i1_to_i64(i1 %src) {
; CHECK-LABEL: @test_i1_to_i64(
; CHECK:    [[ZEXT1:%.*]] = zext i1 [[SRC:%.*]] to i64
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[ZEXT1]] to <2 x i32>
; CHECK:    ret <2 x i32> [[TMP1]]
;
  %1 = zext i1 %src to i64
  %2 = bitcast i64 %1 to <2 x i32>
  ret <2 x i32> %2
}

define double @test_2i32_to_i64_to_df(<2 x i32> %src) {
; CHECK-LABEL: @test_2i32_to_i64_to_df(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i32> [[SRC:%.*]] to double
; CHECK:    ret double [[TMP1]]
;
  %1 = bitcast <2 x i32> %src to i64
  %2 = bitcast i64 %1 to double
  ret double %2
}

define <2 x float> @test_2i32_to_i64_to_2f(<2 x i32> %src) {
; CHECK-LABEL: @test_2i32_to_i64_to_2f(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i32> [[SRC:%.*]] to <2 x float>
; CHECK:    ret <2 x float> [[TMP1]]
;
  %1 = bitcast <2 x i32> %src to i64
  %2 = bitcast i64 %1 to <2 x float>
  ret <2 x float> %2
}
