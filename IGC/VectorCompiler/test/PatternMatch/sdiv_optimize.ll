;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck  --enable-var-scope %s

; COM: test naming:
; COM: sdiv/udiv/srem/urem_b(below zero)/a(above zero)_p2(power of 2)/np2(not power of 2)_type_additional
; COM: example sdiv_b_p2_i32 - sdiv, divisor is power of 2 contant below zero, type i32

; CHECK-LABEL: @sdiv_a_np2_v4i32
define <4 x i32> @sdiv_a_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = sdiv <4 x i32> %op0, <i32 131, i32 131, i32 131, i32 131>

  %out = sdiv <4 x i32> %op0, <i32 131, i32 131, i32 131, i32 131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @sdiv_b_np2_v4i32
define <4 x i32> @sdiv_b_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = sdiv <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  %out = sdiv <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @sdiv_b_p2_v4i32
define <4 x i32> @sdiv_b_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = sdiv <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  %out = sdiv <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @sdiv_a_p2_v4i32
define <4 x i32> @sdiv_a_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i32> %op0, <i32 31, i32 31, i32 31, i32 31>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i32> [[VSIGN]], <i32 25, i32 26, i32 27, i32 28>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i32> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VRES:%[^ ]+]] = ashr <4 x i32> [[VTMP:%[^ ]+]], <i32 7, i32 6, i32 5, i32 4>
  ;CHECK-NEXT: ret <4 x i32> [[VRES]]

  %out = sdiv <4 x i32> %op0, <i32 128, i32 64, i32 32, i32 16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @sdiv_a_p2_v4i16
define <4 x i16> @sdiv_a_p2_v4i16(<4 x i16> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i16> %op0, <i16 15, i16 15, i16 15, i16 15>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i16> [[VSIGN]], <i16 9, i16 10, i16 11, i16 12>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i16> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VRES:%[^ ]+]] = ashr <4 x i16> [[VTMP:%[^ ]+]], <i16 7, i16 6, i16 5, i16 4>
  ;CHECK-NEXT: ret <4 x i16> [[VRES]]

  %out = sdiv <4 x i16> %op0, <i16 128, i16 64, i16 32, i16 16>

  ret <4 x i16> %out
}

; CHECK-LABEL: @sdiv_a_p2_v4i64
define <4 x i64> @sdiv_a_p2_v4i64(<4 x i64> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i64> %op0, <i64 63, i64 63, i64 63, i64 63>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i64> [[VSIGN]], <i64 57, i64 58, i64 59, i64 60>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i64> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VRES:%[^ ]+]] = ashr <4 x i64> [[VTMP:%[^ ]+]], <i64 7, i64 6, i64 5, i64 4>
  ;CHECK-NEXT: ret <4 x i64> [[VRES]]

  %out = sdiv <4 x i64> %op0, <i64 128, i64 64, i64 32, i64 16>

  ret <4 x i64> %out
}

; CHECK-LABEL: @sdiv_a_np2_i32
define i32 @sdiv_a_np2_i32(i32 %op0)  {
  ;CHECK: %out = sdiv i32 %op0, 131

  %out = sdiv i32 %op0, 131

  ret i32 %out
}

; CHECK-LABEL: @sdiv_b_np2_i32
define i32 @sdiv_b_np2_i32(i32 %op0)  {
  ;CHECK: %out = sdiv i32 %op0, -131

  %out = sdiv i32 %op0, -131

  ret i32 %out
}

; CHECK-LABEL: @sdiv_b_p2_i32
define i32 @sdiv_b_p2_i32(i32 %op0)  {
  ;CHECK: %out = sdiv i32 %op0, -16

  %out = sdiv i32 %op0, -16

  ret i32 %out
}

; CHECK-LABEL: @sdiv_a_p2_i32
define i32 @sdiv_a_p2_i32(i32 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i32 %op0, 31
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i32 [[SSIGN]], 28
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i32 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SRET:%[^ ]+]] = ashr i32 [[STMP]], 4
  ;CHECK-NEXT: ret i32 [[SRET]]

  %out = sdiv i32 %op0,  16

  ret i32 %out
}

; CHECK-LABEL: @sdiv_a_p2_i64
define i64 @sdiv_a_p2_i64(i64 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i64 %op0, 63
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i64 [[SSIGN]], 60
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i64 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SRET:%[^ ]+]] = ashr i64 [[STMP]], 4
  ;CHECK-NEXT: ret i64 [[SRET]]

  %out = sdiv i64 %op0, 16

  ret i64 %out
}

; CHECK-LABEL: @sdiv_a_p2_i16
define i16 @sdiv_a_p2_i16(i16 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i16 %op0, 15
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i16 [[SSIGN]], 12
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i16 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SRET:%[^ ]+]] = ashr i16 [[STMP]], 4
  ;CHECK-NEXT: ret i16 [[SRET]]

  %out = sdiv i16 %op0, 16

  ret i16 %out
}
