;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-promoteint8type -S < %s | FileCheck %s

; A promoted i8 result only has a meaningful low byte: -128 + -128 is 0 as
; i8, but 0xFF00 computed on sign-extended operands. Used as a shift amount,
; it has to be zero-extended again from its low byte.

; CHECK-LABEL: @test_shl(
; CHECK:         [[ADD:%.*]] = add i16
; CHECK:         [[TRUNC:%.*]] = trunc i16 [[ADD]] to i8
; CHECK:         [[AMT:%.*]] = zext i8 [[TRUNC]] to i16
; CHECK:         shl i16 {{%.*}}, [[AMT]]
define i8 @test_shl(i8 %x, i8 %a, i8 %b) {
  %amt = add i8 %a, %b
  %r = shl i8 %x, %amt
  ret i8 %r
}

; CHECK-LABEL: @test_lshr(
; CHECK:         [[ADD:%.*]] = add i16
; CHECK:         [[TRUNC:%.*]] = trunc i16 [[ADD]] to i8
; CHECK:         [[AMT:%.*]] = zext i8 [[TRUNC]] to i16
; CHECK:         lshr i16 {{%.*}}, [[AMT]]
define i8 @test_lshr(i8 %x, i8 %a, i8 %b) {
  %amt = add i8 %a, %b
  %r = lshr i8 %x, %amt
  ret i8 %r
}

; CHECK-LABEL: @test_ashr(
; CHECK:         [[ADD:%.*]] = add i16
; CHECK:         [[TRUNC:%.*]] = trunc i16 [[ADD]] to i8
; CHECK:         [[AMT:%.*]] = zext i8 [[TRUNC]] to i16
; CHECK:         ashr i16 {{%.*}}, [[AMT]]
define i8 @test_ashr(i8 %x, i8 %a, i8 %b) {
  %amt = add i8 %a, %b
  %r = ashr i8 %x, %amt
  ret i8 %r
}
