;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promoteint8type -S < %s | FileCheck %s

define i8 @negative_or(i8 %value) {
; CHECK-LABEL: @negative_or(
; CHECK: [[EXT:%.*]] = sext i8 %value to i16
; CHECK: [[RESULT:%.*]] = or i16 [[EXT]], -10
; CHECK: [[BYTE:%.*]] = trunc i16 [[RESULT]] to i8
; CHECK: ret i8 [[BYTE]]
  %result = or i8 %value, -10
  ret i8 %result
}

define i8 @negative_add(i8 %value) {
; CHECK-LABEL: @negative_add(
; CHECK: [[EXT:%.*]] = sext i8 %value to i16
; CHECK: [[RESULT:%.*]] = add i16 [[EXT]], -1
; CHECK: [[BYTE:%.*]] = trunc i16 [[RESULT]] to i8
; CHECK: ret i8 [[BYTE]]
  %result = add i8 %value, -1
  ret i8 %result
}

define i8 @signed_limits(i1 %condition) {
; CHECK-LABEL: @signed_limits(
; CHECK: [[RESULT:%.*]] = select i1 %condition, i16 -128, i16 127
; CHECK: [[BYTE:%.*]] = trunc i16 [[RESULT]] to i8
; CHECK: ret i8 [[BYTE]]
  %result = select i1 %condition, i8 -128, i8 127
  ret i8 %result
}

define i8 @constant_data_vector(i32 %index) {
; CHECK-LABEL: @constant_data_vector(
; CHECK: [[RESULT:%.*]] = extractelement <4 x i16> <i16 -128, i16 -1, i16 0, i16 127>, i32 %index
; CHECK: [[BYTE:%.*]] = trunc i16 [[RESULT]] to i8
; CHECK: ret i8 [[BYTE]]
  %result = extractelement <4 x i8> <i8 -128, i8 -1, i8 0, i8 127>, i32 %index
  ret i8 %result
}

define i8 @constant_vector(i32 %index) {
; CHECK-LABEL: @constant_vector(
; CHECK: [[RESULT:%.*]] = extractelement <4 x i16> <i16 -128, i16 undef, i16 -1, i16 127>, i32 %index
; CHECK: [[BYTE:%.*]] = trunc i16 [[RESULT]] to i8
; CHECK: ret i8 [[BYTE]]
  %result = extractelement <4 x i8> <i8 -128, i8 undef, i8 -1, i8 127>, i32 %index
  ret i8 %result
}

define i8 @unsigned_index(<256 x i8> %values, i32 %index) {
; CHECK-LABEL: @unsigned_index(
; CHECK: extractelement <256 x i16> %{{.*}}, i16 255
  %value = extractelement <256 x i8> %values, i32 %index
  %last = extractelement <256 x i8> %values, i8 -1
  %result = add i8 %value, %last
  ret i8 %result
}
