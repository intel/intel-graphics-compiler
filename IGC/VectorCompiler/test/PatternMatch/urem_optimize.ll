;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck --enable-var-scope %s

; COM: test naming:
; COM: sdiv/udiv/srem/urem_b(below zero)/a(above zero)_p2(power of 2)/np2(not power of 2)_type_additional
; COM: example sdiv_b_p2_i32 - sdiv, divisor is power of 2 contant below zero, type i32

; CHECK-LABEL: @urem_a_np2_v4i32
define <4 x i32> @urem_a_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK:  [[MULH:%[^ ]+]] = call <4 x i32> @llvm.genx.umulh.v4i32.v4i32(<4 x i32> %op0, <4 x i32> <i32 -98358029, i32 -98358029, i32 -98358029, i32 -98358029>)
  ;CHECK-NEXT: [[SHIFT:%[^ ]+]] = lshr <4 x i32> [[MULH]], <i32 7, i32 7, i32 7, i32 7>
  ;CHECK-NEXT: [[MUL:%[^ ]+]] = mul <4 x i32> <i32 131, i32 131, i32 131, i32 131>, [[SHIFT]]
  ;CHECK-NEXT: [[RES:%[^ ]+]] = sub <4 x i32> %op0, [[MUL]]
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %out = urem <4 x i32> %op0, <i32 131, i32 131, i32 131, i32 131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @urem_b_np2_v4i32
define <4 x i32> @urem_b_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = urem <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>
  ;CHECK-NEXT: ret <4 x i32> %out

  %out = urem <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @urem_b_p2_v4i32
define <4 x i32> @urem_b_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = urem <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>
  ;CHECK-NEXT: ret <4 x i32> %out

  %out = urem <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @urem_a_p2_v4i32
define <4 x i32> @urem_a_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and <4 x i32> %op0, <i32 127, i32 63, i32 31, i32 15>
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %out = urem <4 x i32> %op0, <i32 128, i32 64, i32 32, i32 16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @urem_a_p2_v4i64
define <4 x i64> @urem_a_p2_v4i64(<4 x i64> %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and <4 x i64> %op0, <i64 127, i64 63, i64 31, i64 15>
  ;CHECK-NEXT: ret <4 x i64> [[RES]]

  %out = urem <4 x i64> %op0, <i64 128, i64 64, i64 32, i64 16>

  ret <4 x i64> %out
}

; CHECK-LABEL: @urem_a_p2_v4i16
define <4 x i16> @urem_a_p2_v4i16(<4 x i16> %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and <4 x i16> %op0, <i16 127, i16 63, i16 31, i16 15>
  ;CHECK-NEXT: ret <4 x i16> [[RES]]

  %out = urem <4 x i16> %op0, <i16 128, i16 64, i16 32, i16 16>

  ret <4 x i16> %out
}

; CHECK-LABEL: @urem_a_np2_i32
define i32 @urem_a_np2_i32(i32 %op0)  {
  ;CHECK: [[MULH:%[^ ]+]] = call i32 @llvm.genx.umulh.i32.i32(i32 %op0, i32 -98358029)
  ;CHECK-NEXT: [[SHIFT:%[^ ]+]] = lshr i32 [[MULH]], 7
  ;CHECK-NEXT: [[MUL:%[^ ]+]] = mul i32 131, [[SHIFT]]
  ;CHECK-NEXT: [[RES:%[^ ]+]] = sub i32 %op0, [[MUL]]
  ;CHECK-NEXT: ret i32 [[RES]]
  %out = urem i32 %op0, 131

  ret i32 %out
}

; CHECK-LABEL: @urem_b_np2_i32
define i32 @urem_b_np2_i32(i32 %op0)  {
  ;CHECK: %out = urem i32 %op0, -131
  ;CHECK-NEXT: ret i32 %out

  %out = urem i32 %op0, -131

  ret i32 %out
}

; CHECK-LABEL: @urem_b_p2_i32
define i32 @urem_b_p2_i32(i32 %op0)  {
  ;CHECK: %out = urem i32 %op0, -16
  ;CHECK-NEXT: ret i32 %out

  %out = urem i32 %op0, -16

  ret i32 %out
}

; CHECK-LABEL: @urem_a_p2_i32
define i32 @urem_a_p2_i32(i32 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and i32 %op0, 15
  ;CHECK-NEXT: ret i32 [[RES]]

  %out = urem i32 %op0,  16

  ret i32 %out
}

; CHECK-LABEL: @urem_a_p2_i64
define i64 @urem_a_p2_i64(i64 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and i64 %op0, 15
  ;CHECK-NEXT: ret i64 [[RES]]

  %out = urem i64 %op0, 16
  ret i64 %out
}

; CHECK-LABEL: @urem_a_p2_i16
define i16 @urem_a_p2_i16(i16 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = and i16 %op0, 15
  ;CHECK-NEXT: ret i16 [[RES]]

  %out = urem i16 %op0, 16
  ret i16 %out
}
