;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-canonicalize-mul-add -S < %s 2>&1 | FileCheck %s

; Tests for CanonicalizeMulAdd::visitTwoMads.

; Test naming: x_add_one_mul_y_add_one_mul_x => ((x + 1) * (y + 1)) * x

; CHECK-LABEL: @x_add_one_mul_y_add_one_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_one_mul_y_add_one_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %x, 1
  %op2 = add i32 %y, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_mone_mul_y_add_one_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_mone_mul_y_add_one_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %x, -1
  %op2 = add i32 %y, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_one_mul_y_add_mone_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_one_mul_y_add_mone_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %x, 1
  %op2 = add i32 %y, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_mone_mul_y_add_mone_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_mone_mul_y_add_mone_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %x, -1
  %op2 = add i32 %y, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @y_add_one_mul_x_add_one_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @y_add_one_mul_x_add_one_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %y, 1
  %op2 = add i32 %x, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @y_add_mone_mul_x_add_one_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @y_add_mone_mul_x_add_one_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %y, -1
  %op2 = add i32 %x, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @y_add_one_mul_x_add_mone_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @y_add_one_mul_x_add_mone_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %y, 1
  %op2 = add i32 %x, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @y_add_mone_mul_x_add_mone_mul_x
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %x
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @y_add_mone_mul_x_add_mone_mul_x(i32 %x, i32 %y) {
  %op1 = add i32 %y, -1
  %op2 = add i32 %x, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %x, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_one_mul_y_add_one_mul_z
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %z, %x
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %z
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %y
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_one_mul_y_add_one_mul_z(i32 %x, i32 %y, i32 %z) {
  %op1 = add i32 %x, 1
  %op2 = add i32 %y, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %z, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_mone_mul_y_add_one_mul_z
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %z, %x
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %z
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %y
; CHECK-NEXT:  [[MAD2:%.*]] = add i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_mone_mul_y_add_one_mul_z(i32 %x, i32 %y, i32 %z) {
  %op1 = add i32 %x, -1
  %op2 = add i32 %y, 1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %z, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_one_mul_y_add_mone_mul_z
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %z, %x
; CHECK-NEXT:  [[MAD1:%.*]] = add i32 [[MUL1]], %z
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %y
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_one_mul_y_add_mone_mul_z(i32 %x, i32 %y, i32 %z) {
  %op1 = add i32 %x, 1
  %op2 = add i32 %y, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %z, %mul1
  ret i32 %mul2
}

; CHECK-LABEL: @x_add_mone_mul_y_add_mone_mul_z
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %z, %x
; CHECK-NEXT:  [[MAD1:%.*]] = sub i32 [[MUL1]], %z
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[MAD1]], %y
; CHECK-NEXT:  [[MAD2:%.*]] = sub i32 [[MUL2]], [[MAD1]]
; CHECK-NEXT:  ret i32 [[MAD2]]
define i32 @x_add_mone_mul_y_add_mone_mul_z(i32 %x, i32 %y, i32 %z) {
  %op1 = add i32 %x, -1
  %op2 = add i32 %y, -1
  %mul1 = mul i32 %op1, %op2
  %mul2 = mul i32 %z, %mul1
  ret i32 %mul2
}
