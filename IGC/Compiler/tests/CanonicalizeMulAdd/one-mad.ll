;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-canonicalize-mul-add -S < %s 2>&1 | FileCheck %s

; Tests for CanonicalizeMulAdd::visitOneMad.

; Test naming: x_add_one_mul_y=> (x + 1) * y

; CHECK-LABEL: @x_add_one_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[ADD:%.*]] = add i32 [[MUL]], %y
; CHECK-NEXT:  ret i32 [[ADD]]
define i32 @x_add_one_mul_y(i32 %x, i32 %y) {
  %add = add i32 %x, 1
  %mul = mul i32 %y, %add
  ret i32 %mul
}

; CHECK-LABEL: @x_add_mone_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 [[MUL]], %y
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @x_add_mone_mul_y(i32 %x, i32 %y) {
  %add = add i32 %x, -1
  %mul = mul i32 %y, %add
  ret i32 %mul
}

; CHECK-LABEL: @mone_add_x_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 [[MUL]], %y
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @mone_add_x_mul_y(i32 %x, i32 %y) {
  %add = add i32 -1, %x
  %mul = mul i32 %y, %add
  ret i32 %mul
}

; CHECK-LABEL: @x_sub_one_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 [[MUL]], %y
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @x_sub_one_mul_y(i32 %x, i32 %y) {
  %sub = sub i32 %x, 1
  %mul = mul i32 %y, %sub
  ret i32 %mul
}

; CHECK-LABEL: @x_sub_mone_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[ADD:%.*]] = add i32 [[MUL]], %y
; CHECK-NEXT:  ret i32 [[ADD]]
define i32 @x_sub_mone_mul_y(i32 %x, i32 %y) {
  %sub = sub i32 %x, -1
  %mul = mul i32 %y, %sub
  ret i32 %mul
}

; CHECK-LABEL: @one_sub_x_mul_y
; CHECK-NEXT:  [[MUL:%.*]] = mul i32 %y, %x
; CHECK-NEXT:  [[SUB:%.*]] = sub i32 %y, [[MUL]]
; CHECK-NEXT:  ret i32 [[SUB]]
define i32 @one_sub_x_mul_y(i32 %x, i32 %y) {
  %sub = sub i32 1, %x
  %mul = mul i32 %y, %sub
  ret i32 %mul
}
