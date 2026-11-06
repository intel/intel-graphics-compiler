;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-promoteint8type -S < %s | FileCheck %s

; A promoted i8 result only has a meaningful low byte: 1 - 2 is 255 as i8,
; but 0xFFFF computed on sign-extended operands. Used as a dynamic vector
; index, it has to be zero-extended again from its low byte.

; CHECK-LABEL: @test_promoted_index(
; CHECK:         [[SUB:%.*]] = sub i16
; CHECK:         [[TRUNC:%.*]] = trunc i16 [[SUB]] to i8
; CHECK:         [[IDX:%.*]] = zext i8 [[TRUNC]] to i16
; CHECK:         extractelement <10 x i16> {{%.*}}, i16 [[IDX]]
define i8 @test_promoted_index(<10 x i8> %v, i8 %x, i8 %y) {
  %idx = sub i8 %x, %y
  %e = extractelement <10 x i8> %v, i8 %idx
  ret i8 %e
}

; An i32 vector isn't promoted, the index is the truncated i8 value.
; CHECK-LABEL: @test_promoted_index_i32_vector(
; CHECK:         [[SUB:%.*]] = sub i16
; CHECK:         [[TRUNC:%.*]] = trunc i16 [[SUB]] to i8
; CHECK:         extractelement <10 x i32> %v, i8 [[TRUNC]]
define i32 @test_promoted_index_i32_vector(<10 x i32> %v, i8 %x, i8 %y) {
  %idx = sub i8 %x, %y
  %e = extractelement <10 x i32> %v, i8 %idx
  ret i32 %e
}
