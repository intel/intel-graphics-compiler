;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: bitcast to bool (i1) vector
; ------------------------------------------------

; On LLVM 22, InstCombine folds a trunc-to-i1 of an extracted element into
; a bitcast to a bool vector plus an extractelement. IGC represents i1 as
; a HW flag and cannot alias a wide vector as a bool vector.
; Legalization now rewrites the extract into an element extract + shift + mask + trunc.

define i1 @test_bit0(<8 x i32> %v) {
; CHECK-LABEL: define i1 @test_bit0(
; CHECK-SAME: <8 x i32> [[V:%.*]]) {
; CHECK:    [[E:%.*]] = extractelement <8 x i32> [[V]], i32 0
; CHECK:    [[A:%.*]] = and i32 [[E]], 1
; CHECK:    [[T:%.*]] = trunc i32 [[A]] to i1
; CHECK:    ret i1 [[T]]
;
  %bc = bitcast <8 x i32> %v to <256 x i1>
  %b = extractelement <256 x i1> %bc, i64 0
  ret i1 %b
}

define i1 @test_bit64(<8 x i32> %v) {
; CHECK-LABEL: define i1 @test_bit64(
; CHECK-SAME: <8 x i32> [[V:%.*]]) {
; CHECK:    [[E:%.*]] = extractelement <8 x i32> [[V]], i32 2
; CHECK:    [[A:%.*]] = and i32 [[E]], 1
; CHECK:    [[T:%.*]] = trunc i32 [[A]] to i1
; CHECK:    ret i1 [[T]]
;
  %bc = bitcast <8 x i32> %v to <256 x i1>
  %b = extractelement <256 x i1> %bc, i64 64
  ret i1 %b
}

define i1 @test_bit65(<8 x i32> %v) {
; CHECK-LABEL: define i1 @test_bit65(
; CHECK-SAME: <8 x i32> [[V:%.*]]) {
; CHECK:    [[E:%.*]] = extractelement <8 x i32> [[V]], i32 2
; CHECK:    [[S:%.*]] = lshr i32 [[E]], 1
; CHECK:    [[A:%.*]] = and i32 [[S]], 1
; CHECK:    [[T:%.*]] = trunc i32 [[A]] to i1
; CHECK:    ret i1 [[T]]
;
  %bc = bitcast <8 x i32> %v to <256 x i1>
  %b = extractelement <256 x i1> %bc, i64 65
  ret i1 %b
}

; Same illegal bool-vector bitcast, but reached via LegalizeGVNBitCastPattern's
; scalar-i128 path (bitcast <4 x float> to i128; lshr; trunc to i1): match3
; rewrites it into a bitcast to <128 x i1>, which this pass then legalizes.
define i1 @test_gvn_scalar_i128(<4 x float> %src1) {
; CHECK-LABEL: define i1 @test_gvn_scalar_i128(
; CHECK-SAME: <4 x float> [[SRC1:%.*]]) {
; CHECK:    [[E:%.*]] = extractelement <4 x float> [[SRC1]], i32 2
; CHECK:    [[BC:%.*]] = bitcast float [[E]] to i32
; CHECK:    [[S:%.*]] = lshr i32 [[BC]], 1
; CHECK:    [[A:%.*]] = and i32 [[S]], 1
; CHECK:    [[T:%.*]] = trunc i32 [[A]] to i1
; CHECK:    ret i1 [[T]]
;
  %1 = bitcast <4 x float> %src1 to i128
  %2 = lshr i128 %1, 65
  %3 = trunc i128 %2 to i1
  ret i1 %3
}

; No bitcast-to-bool-vector left behind.
; CHECK-NOT: to <{{[0-9]+}} x i1>

!igc.functions = !{!0, !1, !2, !3}

!0 = !{i1 (<8 x i32>)* @test_bit0, !4}
!1 = !{i1 (<8 x i32>)* @test_bit64, !4}
!2 = !{i1 (<8 x i32>)* @test_bit65, !4}
!3 = !{i1 (<4 x float>)* @test_gvn_scalar_i128, !4}
!4 = !{}
