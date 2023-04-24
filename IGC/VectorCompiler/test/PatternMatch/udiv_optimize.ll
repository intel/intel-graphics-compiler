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

; CHECK-LABEL: @udiv_a_np2_v4i32_3
define <4 x i32> @udiv_a_np2_v4i32_3(<4 x i32> %op0)  {
  ;CHECK: [[MULH:%[^ ]+]] = call <4 x i32> @llvm.genx.umulh.v4i32.v4i32(<4 x i32> %op0, <4 x i32> <i32 -1431655765, i32 -1431655765, i32 -1431655765, i32 -1431655765>)
  ;CHECK-NEXT: [[SHIFT:%[^ ]+]] = lshr <4 x i32> [[MULH]], <i32 1, i32 1, i32 1, i32 1>
  ;CHECK-NEXT: ret <4 x i32> [[SHIFT]]

  %out = udiv <4 x i32> %op0, <i32 3, i32 3, i32 3, i32 3>

  ret <4 x i32> %out
}

; CHECK-LABEL: @udiv_a_np2_v4i32_7
define <4 x i32> @udiv_a_np2_v4i32_7(<4 x i32> %op0)  {
  ;CHECK: [[MULH:%[^ ]+]] = call <4 x i32> @llvm.genx.umulh.v4i32.v4i32(<4 x i32> %op0, <4 x i32> <i32 613566757, i32 613566757, i32 613566757, i32 613566757>)
  ;CHECK-NEXT: [[SUB:%[^ ]+]] = sub <4 x i32> %op0, [[MULH]]
  ;CHECK-NEXT: [[LSHIFT1:%[^ ]+]] = lshr <4 x i32> [[SUB]], <i32 1, i32 1, i32 1, i32 1>
  ;CHECK-NEXT: [[ADD:%[^ ]+]] = add <4 x i32> [[LSHIFT1]], %opt
  ;CHECK-NEXT: [[RES:%[^ ]+]] = lshr <4 x i32> [[ADD]], <i32 2, i32 2, i32 2, i32 2>
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %out = udiv <4 x i32> %op0, <i32 7, i32 7, i32 7, i32 7>

  ret <4 x i32> %out
}

; CHECK-LABEL: @udiv_b_np2_v4i32
define <4 x i32> @udiv_b_np2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = udiv <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>
  ;CHECK-NEXT: ret <4 x i32> %out

  %out = udiv <4 x i32> %op0, <i32 -131, i32 -131, i32 -131, i32 -131>

  ret <4 x i32> %out
}

; CHECK-LABEL: @udiv_b_p2_v4i32
define <4 x i32> @udiv_b_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: %out = udiv <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>
  ;CHECK-NEXT: ret <4 x i32> %out

  %out = udiv <4 x i32> %op0, <i32 -128, i32 -64, i32 -32, i32 -16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @udiv_a_p2_v4i32
define <4 x i32> @udiv_a_p2_v4i32(<4 x i32> %op0)  {
  ;CHECK: [[SHIFT:%[^ ]+]] = lshr <4 x i32> %op0, <i32 7, i32 6, i32 5, i32 4>
  ;CHECK-NEXT: ret <4 x i32> [[SHIFT]]

  %out = udiv <4 x i32> %op0, <i32 128, i32 64, i32 32, i32 16>

  ret <4 x i32> %out
}

; CHECK-LABEL: @udiv_a_p2_v4i64
define <4 x i64> @udiv_a_p2_v4i64(<4 x i64> %op0)  {
  ;CHECK: [[SHIFT:%[^ ]+]] = lshr <4 x i64> %op0, <i64 7, i64 6, i64 5, i64 4>
  ;CHECK-NEXT: ret <4 x i64> [[SHIFT]]

  %out = udiv <4 x i64> %op0, <i64 128, i64 64, i64 32, i64 16>
  ret <4 x i64> %out
}

; CHECK-LABEL: @udiv_a_p2_v4i16
define <4 x i16> @udiv_a_p2_v4i16(<4 x i16> %op0)  {
  ;CHECK: [[SHIFT:%[^ ]+]] = lshr <4 x i16> %op0, <i16 7, i16 6, i16 5, i16 4>
  ;CHECK-NEXT: ret <4 x i16> [[SHIFT]]

  %out = udiv <4 x i16> %op0, <i16 128, i16 64, i16 32, i16 16>

  ret <4 x i16> %out
}

; CHECK-LABEL: @udiv_a_np2_i32
define i32 @udiv_a_np2_i32(i32 %op0)  {
  ;CHECK: [[MULH:%[^ ]+]] = call i32 @llvm.genx.umulh.i32.i32(i32 %op0, i32 -98358029)
  ;CHECK-NEXT: [[RES:%[^ ]+]] = lshr i32 [[MULH]], 7
  ;CHECK-NEXT: ret i32 [[RES]]

  %out = udiv i32 %op0, 131

  ret i32 %out
}

; CHECK-LABEL: @udiv_b_np2_i32
define i32 @udiv_b_np2_i32(i32 %op0)  {
  ;CHECK: %out = udiv i32 %op0, -131
  ;CHECK-NEXT: ret i32 %out

  %out = udiv i32 %op0, -131

  ret i32 %out
}

; CHECK-LABEL: @udiv_b_p2_i32
define i32 @udiv_b_p2_i32(i32 %op0)  {
  ;CHECK: %out = udiv i32 %op0, -16
  ;CHECK-NEXT: ret i32 %out

  %out = udiv i32 %op0, -16

  ret i32 %out
}

; CHECK-LABEL: @udiv_a_p2_i32
define i32 @udiv_a_p2_i32(i32 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = lshr i32 %op0, 4
  ;CHECK-NEXT: ret i32 [[RES]]

  %out = udiv i32 %op0,  16

  ret i32 %out
}

; CHECK-LABEL: @udiv_a_p2_i64
define i64 @udiv_a_p2_i64(i64 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = lshr i64 %op0, 4
  ;CHECK-NEXT: ret i64 [[RES]]

  %out = udiv i64 %op0, 16

  ret i64 %out
}

; CHECK-LABEL: @udiv_a_p2_i16
define i16 @udiv_a_p2_i16(i16 %op0)  {
  ;CHECK: [[RES:%[^ ]+]] = lshr i16 %op0, 4
  ;CHECK-NEXT: ret i16 [[RES]]

  %out = udiv i16 %op0, 16

  ret i16 %out
}
