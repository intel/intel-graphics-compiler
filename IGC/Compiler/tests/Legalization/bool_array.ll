;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-int-type-legalizer -S < %s | FileCheck %s
;
; Tests for array of i1 types.

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define i8 @test_x_on_least_significant_index_plus_zeros_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_x_on_least_significant_index_plus_zeros_constants(
; CHECK:    [[V0:%.*]] = select i1 %a, i8 1, i8 0
; CHECK:    [[V1_tmp:%.*]] = select i1 %b, i8 1, i8 0
; CHECK:    [[V1:%.*]] = shl i8 [[V1_tmp]], 1
; CHECK:    [[RESULT:%.*]] = or i8 [[V0]], [[V1]]
; CHECK:    ret i8 [[RESULT]]
;
  %1 = insertelement <8 x i1> undef, i1 %a, i32 0
  %2 = insertelement <8 x i1> %1, i1 %b, i32 1
  %3 = insertelement <8 x i1> %2, i1 false, i32 2
  %4 = insertelement <8 x i1> %3, i1 false, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 false, i32 5
  %7 = insertelement <8 x i1> %6, i1 false, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}

define i8 @test_x_on_least_significant_index_plus_random_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_x_on_least_significant_index_plus_random_constants(
; CHECK:    [[V0:%.*]] = select i1 %a, i8 1, i8 0
; CHECK:    [[RESULT0:%.*]] = or i8 40, [[V0]]
; CHECK:    [[V1_tmp:%.*]] = select i1 %b, i8 1, i8 0
; CHECK:    [[V1:%.*]] = shl i8 [[V1_tmp]], 1
; CHECK:    [[RESULT1:%.*]] = or i8 [[RESULT0]], [[V1]]
; CHECK:    ret i8 [[RESULT1]]
;
  %1 = insertelement <8 x i1> undef, i1 %a, i32 0
  %2 = insertelement <8 x i1> %1, i1 %b, i32 1
  %3 = insertelement <8 x i1> %2, i1 false, i32 2
  %4 = insertelement <8 x i1> %3, i1 true, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 true, i32 5
  %7 = insertelement <8 x i1> %6, i1 false, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}

define i8 @test_x_on_random_index_plus_zeros_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_x_on_random_index_plus_zeros_constants(
; CHECK:    [[V0_tmp:%.*]] = select i1 %a, i8 1, i8 0
; CHECK:    [[V0:%.*]] = shl i8 [[V0_tmp]], 2
; CHECK:    [[V1_tmp:%.*]] = select i1 %b, i8 1, i8 0
; CHECK:    [[V1:%.*]] = shl i8 [[V1_tmp]], 5
; CHECK:    [[RESULT:%.*]] = or i8 [[V0]], [[V1]]
; CHECK:    ret i8 [[RESULT]]
;
  %1 = insertelement <8 x i1> undef, i1 false, i32 0
  %2 = insertelement <8 x i1> %1, i1 false, i32 1
  %3 = insertelement <8 x i1> %2, i1 %a, i32 2
  %4 = insertelement <8 x i1> %3, i1 false, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 %b, i32 5
  %7 = insertelement <8 x i1> %6, i1 false, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}

define i8 @test_x_on_random_index_plus_random_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_x_on_random_index_plus_random_constants(
; CHECK:    [[V0_tmp:%.*]] = select i1 %a, i8 1, i8 0
; CHECK:    [[V0:%.*]] = shl i8 [[V0_tmp]], 2
; CHECK:    [[RESULT0:%.*]] = or i8 65, [[V0]]
; CHECK:    [[V1_tmp:%.*]] = select i1 %b, i8 1, i8 0
; CHECK:    [[V1:%.*]] = shl i8 [[V1_tmp]], 5
; CHECK:    [[RESULT1:%.*]] = or i8 [[RESULT0]], [[V1]]
; CHECK:    ret i8 [[RESULT1]]
;
  %1 = insertelement <8 x i1> undef, i1 true, i32 0
  %2 = insertelement <8 x i1> %1, i1 false, i32 1
  %3 = insertelement <8 x i1> %2, i1 %a, i32 2
  %4 = insertelement <8 x i1> %3, i1 false, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 %b, i32 5
  %7 = insertelement <8 x i1> %6, i1 true, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}

define i8 @test_no_x_plus_zeros_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_no_x_plus_zeros_constants(
; CHECK:    ret i8 0
;
  %1 = insertelement <8 x i1> undef, i1 false, i32 0
  %2 = insertelement <8 x i1> %1, i1 false, i32 1
  %3 = insertelement <8 x i1> %2, i1 false, i32 2
  %4 = insertelement <8 x i1> %3, i1 false, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 false, i32 5
  %7 = insertelement <8 x i1> %6, i1 false, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}

define i8 @test_no_x_plus_random_constants(i1 %a, i1 %b) {
; CHECK-LABEL: @test_no_x_plus_random_constants(
; CHECK:    ret i8 40
;
  %1 = insertelement <8 x i1> undef, i1 false, i32 0
  %2 = insertelement <8 x i1> %1, i1 false, i32 1
  %3 = insertelement <8 x i1> %2, i1 false, i32 2
  %4 = insertelement <8 x i1> %3, i1 true, i32 3
  %5 = insertelement <8 x i1> %4, i1 false, i32 4
  %6 = insertelement <8 x i1> %5, i1 true, i32 5
  %7 = insertelement <8 x i1> %6, i1 false, i32 6
  %8 = insertelement <8 x i1> %7, i1 false, i32 7
  %9 = bitcast <8 x i1> %8 to i8
  ret i8 %9
}
