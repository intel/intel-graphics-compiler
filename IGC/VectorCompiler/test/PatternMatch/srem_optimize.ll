;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck --enable-var-scope %s

; COM: test naming:
; COM: sdiv/udiv/srem/urem_b(below zero)/a(above zero)_p2(power of 2)/np2(not power of 2)_type_additional
; COM: example sdiv_b_p2_i32 - sdiv, divisor is power of 2 contant below zero, type i32

; CHECK-LABEL: @srem_a_np2_v4i32
define <4 x i32> @srem_a_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = srem <4 x i32> %op0, <i32 131, i32 131, i32 131, i32 131>

  %out = srem <4 x i32> %op0, <i32 131, i32 131, i32 131, i32 131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @srem_b_np2_v4i32
define <4 x i32> @srem_b_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = srem <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  %out = srem <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @srem_b_p2_v4i32
define <4 x i32> @srem_b_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = srem <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  %out = srem <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @srem_a_p2_v4i32
define <4 x i32> @srem_a_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i32> %op0, <i32 31, i32 31, i32 31, i32 31>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i32> [[VSIGN]], <i32 25, i32 26, i32 27, i32 28>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i32> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VDIVRES:%[^ ]+]] = ashr <4 x i32> [[VTMP]], <i32 7, i32 6, i32 5, i32 4>
  ;CHECK-NEXT: [[VDIVIDEND:%[^ ]+]] = shl <4 x i32> [[VDIVRES]], <i32 7, i32 6, i32 5, i32 4>
  ;CHECK-NEXT: %out = sub <4 x i32> %op0, [[VDIVIDEND]]
  ;CHECK-NEXT: ret <4 x i32> %out

  %out = srem <4 x i32> %op0, <i32 128, i32 64, i32 32, i32 16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @srem_a_p2_v4i64
define <4 x i64> @srem_a_p2_v4i64(<4 x i64> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i64> %op0, <i64 63, i64 63, i64 63, i64 63>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i64> [[VSIGN]], <i64 57, i64 58, i64 59, i64 60>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i64> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VDIVRES:%[^ ]+]] = ashr <4 x i64> [[VTMP]], <i64 7, i64 6, i64 5, i64 4>
  ;CHECK-NEXT: [[VDIVIDEND:%[^ ]+]] = shl <4 x i64> [[VDIVRES]], <i64 7, i64 6, i64 5, i64 4>
  ;CHECK-NEXT: %out = sub <4 x i64> %op0, [[VDIVIDEND]]
  ;CHECK-NEXT: ret <4 x i64> %out

  %out = srem <4 x i64> %op0, <i64 128, i64 64, i64 32, i64 16>

  ret <4 x i64> %out
}

; CHECK-LABEL: @srem_a_p2_v4i16
define <4 x i16> @srem_a_p2_v4i16(<4 x i16> %op0)  {
  ;CHECK: [[VSIGN:%[^ ]+]] = ashr <4 x i16> %op0, <i16 15, i16 15, i16 15, i16 15>
  ;CHECK-NEXT: [[VADDOP:%[^ ]+]] = lshr <4 x i16> [[VSIGN]], <i16 9, i16 10, i16 11, i16 12>
  ;CHECK-NEXT: [[VTMP:%[^ ]+]] = add <4 x i16> %op0, [[VADDOP]]
  ;CHECK-NEXT: [[VDIVRES:%[^ ]+]] = ashr <4 x i16> [[VTMP]], <i16 7, i16 6, i16 5, i16 4>
  ;CHECK-NEXT: [[VDIVIDEND:%[^ ]+]] = shl <4 x i16> [[VDIVRES]], <i16 7, i16 6, i16 5, i16 4>
  ;CHECK-NEXT: %out = sub <4 x i16> %op0, [[VDIVIDEND]]
  ;CHECK-NEXT: ret <4 x i16> %out

  %out = srem <4 x i16> %op0, <i16 128, i16 64, i16 32, i16 16>

  ret <4 x i16> %out
}

; CHECK-LABEL: @srem_a_np2_i32
define i32 @srem_a_np2_i32(i32 %op0)  {
  ;CHECK: %out = srem i32 %op0, 131

  %out = srem i32 %op0, 131

  ret i32 %out
}

; CHECK-LABEL: @srem_b_np2_i32
define i32 @srem_b_np2_i32(i32 %op0)  {
  ;CHECK: %out = srem i32 %op0, -131

  %out = srem i32 %op0, -131

  ret i32 %out
}

; CHECK-LABEL: @srem_b_p2_i32
define i32 @srem_b_p2_i32(i32 %op0)  {
  ;CHECK: %out = srem i32 %op0, -16

  %out = srem i32 %op0, -16

  ret i32 %out
}

; CHECK-LABEL: @srem_a_p2_i32
define i32 @srem_a_p2_i32(i32 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i32 %op0, 31
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i32 [[SSIGN]], 28
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i32 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SDIVRES:%[^ ]+]] = ashr i32 [[STMP]], 4
  ;CHECK-NEXT: [[SDIVIDEND:%[^ ]+]] = shl i32 [[SDIVRES]], 4
  ;CHECK-NEXT: %out = sub i32 %op0, [[SDIVIDEND]]
  ;CHECK-NEXT: ret i32 %out

  %out = srem i32 %op0,  16

  ret i32 %out
}

; CHECK-LABEL: @srem_a_p2_i64
define i64 @srem_a_p2_i64(i64 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i64 %op0, 63
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i64 [[SSIGN]], 60
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i64 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SDIVRES:%[^ ]+]] = ashr i64 [[STMP]], 4
  ;CHECK-NEXT: [[SDIVIDEND:%[^ ]+]] = shl i64 [[SDIVRES]], 4
  ;CHECK-NEXT: %out = sub i64 %op0, [[SDIVIDEND]]
  ;CHECK-NEXT: ret i64 %out

  %out = srem i64 %op0, 16

  ret i64 %out
}

; CHECK-LABEL: @srem_a_p2_i16
define i16 @srem_a_p2_i16(i16 %op0)  {
  ;CHECK: [[SSIGN:%[^ ]+]] = ashr i16 %op0, 15
  ;CHECK-NEXT: [[SADDOP:%[^ ]+]] = lshr i16 [[SSIGN]], 12
  ;CHECK-NEXT: [[STMP:%[^ ]+]] = add i16 %op0, [[SADDOP]]
  ;CHECK-NEXT: [[SDIVRES:%[^ ]+]] = ashr i16 [[STMP]], 4
  ;CHECK-NEXT: [[SDIVIDEND:%[^ ]+]] = shl i16 [[SDIVRES]], 4
  ;CHECK-NEXT: %out = sub i16 %op0, [[SDIVIDEND]]
  ;CHECK-NEXT: ret i16 %out

  %out = srem i16 %op0, 16

  ret i16 %out
}
