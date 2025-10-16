;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-canonicalize-mul-add -S < %s 2>&1 | FileCheck %s

; Tests for CanonicalizeMulAdd::visitRotate.

; CHECK-LABEL: @xy_add_left_side
; CHECK-NEXT:  %mul1 = mul i32 %x, %y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %mul1, %y
; CHECK-NEXT:  [[ADD:%.*]] = add i32 [[MUL]], %mul1
; CHECK-NEXT:  ret i32 [[ADD]]
define i32 @xy_add_left_side(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %y
  ret i32 %mul2
}

; CHECK-LABEL: @xy_add_right_side
; CHECK-NEXT:  %mul1 = mul i32 %x, %y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %mul1, %y
; CHECK-NEXT:  [[ADD:%.*]] = add i32 [[MUL]], %mul1
; CHECK-NEXT:  ret i32 [[ADD]]
define i32 @xy_add_right_side(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %x, %mul1
  %mul2 = mul i32 %add1, %y
  ret i32 %mul2
}

; CHECK-LABEL: @xy_sub_left_side
; CHECK-NEXT:  %mul1 = mul i32 %x, %y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %mul1, %y
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 [[MUL]], %mul1
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @xy_sub_left_side(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %y
  ret i32 %mul2
}

; CHECK-LABEL: @xy_sub_right_side
; CHECK-NEXT:  %mul1 = mul i32 %x, %y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %mul1, %y
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 %mul1, [[MUL]]
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @xy_sub_right_side(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %x, %mul1
  %mul2 = mul i32 %sub1, %y
  ret i32 %mul2
}
