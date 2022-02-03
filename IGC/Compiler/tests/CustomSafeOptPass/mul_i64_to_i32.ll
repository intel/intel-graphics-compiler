;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @testkernel1(i64 %x, i64 %y) nounwind {
entry:
  %conv = add i64 %x, 22
  %conv2 = sub i64 %y, 53
  %mul = mul i64 %conv, %conv2
  %conv3 = and i64 %mul, 4294967295

  ret void
}

; CHECK-LABEL: @testkernel1
; CHECK: [[L1:%[a-zA-Z0-9]+]] = trunc i64 %x to i32
; CHECK: [[ADD:%[a-zA-Z0-9]+]] = add i32 [[L1]], 22
; CHECK: [[L2:%[a-zA-Z0-9]+]] = trunc i64 %y to i32
; CHECK: [[SUB:%[a-zA-Z0-9]+]] = sub i32 [[L2]], 53
; CHECK: [[MUL:%[a-zA-Z0-9]+]] = mul i32 [[ADD]], [[SUB]]
; CHECK: [[ZEXT:%[a-zA-Z0-9]+]] = zext i32 [[MUL]] to i64


define void @testkernel2(i64 %x, i64 %y) nounwind {
entry:
  %conv = add i64 %x, 22
  %conv2 = sub i64 %y, 53
  %div = udiv i64 %conv, %conv2
  %mul = mul i64 %x, %y
  %mul2 = mul i64 %div, %mul
  %conv3 = and i64 %mul2, 4294967295

  ret void
}

; CHECK-LABEL: @testkernel2
; CHECK: [[L3:%[a-zA-Z0-9]+]] = trunc i64 %x to i32
; CHECK: [[L4:%[a-zA-Z0-9]+]] = trunc i64 %y to i32
; CHECK: [[MUL1:%[a-zA-Z0-9]+]] = mul i32 [[L3]], [[L4]]
; CHECK: [[L1:%[a-zA-Z0-9]+]] = trunc i64 %div to i32
; CHECK: [[MUL2:%[a-zA-Z0-9]+]] = mul i32 [[L1]], [[MUL1]]
; CHECK: [[ZEXT:%[a-zA-Z0-9]+]] = zext i32 [[MUL2]] to i64

define void @testkernel3(i64 %x, i64 %y) nounwind {
entry:
  %conv = add i64 %x, 22
  %conv2 = sub i64 %y, 53
  %div = udiv i64 %conv, %conv2
  %mul1 = mul i64 %x, %y
  %mul2 = mul i64 %div, %mul1
  %conv3 = and i64 %mul2, 4294967295
  %mul3 = mul i64 %mul2, %conv

  ret void
}

; CHECK-LABEL: @testkernel3
; CHECK: [[MUL1:%[a-zA-Z0-9]+]] = mul i64 {{%[a-zA-Z0-9]+}}, {{%[a-zA-Z0-9]+}}
; CHECK: [[MUL2:%[a-zA-Z0-9]+]] = mul i64 {{%[a-zA-Z0-9]+}}, [[MUL1]]
; CHECK: %{{[a-zA-Z0-9]+}} = and i64 [[MUL2]], 4294967295
; CHECK: %{{[a-zA-Z0-9]+}} = mul i64 [[MUL2]], {{%[a-zA-Z0-9]+}}

define void @testkernel4(i64 %x, i64 %y) nounwind {
entry:
  %conv = add i64 %x, 22
  %conv2 = sub i64 %y, 53
  %div = udiv i64 %conv, %conv2
  %mul = mul i64 %x, %y
  %mul2 = mul i64 %div, %mul
  %conv3 = and i64 %mul2, 4294967295
  %mul3 = mul i64 %div, %y

  ret void
}

; CHECK-LABEL: @testkernel4
; CHECK: [[ADD:%[a-zA-Z0-9]+]] = add i64 %x, 22
; CHECK: [[SUB:%[a-zA-Z0-9]+]] = sub i64 %y, 53
; CHECK: [[UDIV:%[a-zA-Z0-9]+]] = udiv i64 [[ADD]], [[SUB]]
; CHECK: [[L1:%[a-zA-Z0-9]+]] = trunc i64 %x to i32
; CHECK: [[L2:%[a-zA-Z0-9]+]] = trunc i64 %y to i32
; CHECK: [[MUL1:%[a-zA-Z0-9]+]] = mul i32 [[L1]], [[L2]]
; CHECK: [[L3:%[a-zA-Z0-9]+]] = trunc i64 [[UDIV]] to i32
; CHECK: [[MUL2:%[a-zA-Z0-9]+]] = mul i32 [[L3]], [[MUL1]]
; CHECK: [[ZEXT:%[a-zA-Z0-9]+]] = zext i32 [[MUL2]] to i64
; CHECK: %{{[a-zA-Z0-9]+}} = mul i64 [[UDIV]], %y

define void @testkernel5(i64 %x, i64 %y) nounwind {
entry:
  %conv = add i64 %x, 22
  %conv2 = sub i64 %y, 53
  %mul = udiv i64 %conv, %conv2
  %conv3 = and i64 %mul, 42949676

  ret void
}

; CHECK-LABEL: @testkernel5
; CHECK: %{{[a-zA-Z0-9]+}} = and i64 %{{[a-zA-Z0-9]+}}, 42949676

define void @testkernel6(i64 %x, i64 %y) nounwind {
entry:
  %0 = trunc i64 %x to i32
  %1 = trunc i64 %y to i32
  %conv = add i32 %0, %1
  %conv3 = and i32 %conv, 42956334

  ret void
}

; CHECK-LABEL: @testkernel6
; CHECK: %{{[a-zA-Z0-9]+}} = and i32 %{{[a-zA-Z0-9]+}}, 42956334
