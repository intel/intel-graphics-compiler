;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-type-legalizer -verify -S %s -o - | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-LLVM22%} %else %{CHECK-PRE22%}

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; Promoted operands are sign-extended for smin/smax and zero-extended for
; umin/umax, as the bits above the original width are undefined.

define i64 @f1(i64 %a, i64 %b) {
  %ta = trunc i64 %a to i40
  %tb = trunc i64 %b to i40
  %min = call i40 @llvm.smin.i40(i40 %ta, i40 %tb)
  %r = sext i40 %min to i64
  ret i64 %r
}

; CHECK-LABEL: define i64 @f1
; CHECK: %1 = and i64 %a, 1099511627775
; CHECK: %2 = and i64 %b, 1099511627775
; CHECK: [[ASHL:%.*]] = shl i64 %1, 24
; CHECK: [[ASEXT:%.*]] = ashr i64 [[ASHL]], 24
; CHECK: [[BSHL:%.*]] = shl i64 %2, 24
; CHECK: [[BSEXT:%.*]] = ashr i64 [[BSHL]], 24
; CHECK: [[MIN:%.*]] = call i64 @llvm.smin.i64(i64 [[ASEXT]], i64 [[BSEXT]])
; CHECK: [[RSHL:%.*]] = shl i64 [[MIN]], 24
; CHECK: [[R:%.*]] = ashr i64 [[RSHL]], 24
; CHECK: ret i64 [[R]]

define i32 @f2(i32 %a, i32 %b) {
  %ta = trunc i32 %a to i31
  %tb = trunc i32 %b to i31
  %max = call i31 @llvm.smax.i31(i31 %ta, i31 %tb)
  %r = sext i31 %max to i32
  ret i32 %r
}

; CHECK-LABEL: define i32 @f2
; CHECK: %1 = and i32 %a, 2147483647
; CHECK: %2 = and i32 %b, 2147483647
; CHECK: [[ASHL:%.*]] = shl i32 %1, 1
; CHECK: [[ASEXT:%.*]] = ashr i32 [[ASHL]], 1
; CHECK: [[BSHL:%.*]] = shl i32 %2, 1
; CHECK: [[BSEXT:%.*]] = ashr i32 [[BSHL]], 1
; CHECK: [[MAX:%.*]] = call i32 @llvm.smax.i32(i32 [[ASEXT]], i32 [[BSEXT]])
; CHECK: [[RSHL:%.*]] = shl i32 [[MAX]], 1
; CHECK: [[R:%.*]] = ashr i32 [[RSHL]], 1
; CHECK: ret i32 [[R]]

define i16 @f3(i16 %a, i16 %b) {
  %ta = trunc i16 %a to i9
  %tb = trunc i16 %b to i9
  %min = call i9 @llvm.umin.i9(i9 %ta, i9 %tb)
  %r = zext i9 %min to i16
  ret i16 %r
}

; CHECK-LABEL: define i16 @f3
; CHECK: %1 = and i16 %a, 511
; CHECK: %2 = and i16 %b, 511
; CHECK: [[AZEXT:%.*]] = and i16 %1, 511
; CHECK: [[BZEXT:%.*]] = and i16 %2, 511
; CHECK: [[MIN:%.*]] = call i16 @llvm.umin.i16(i16 [[AZEXT]], i16 [[BZEXT]])
; CHECK: [[R:%.*]] = and i16 [[MIN]], 511
; CHECK: ret i16 [[R]]


define i8 @f4(i8 %a, i8 %b) {
  %ta = trunc i8 %a to i3
  %tb = trunc i8 %b to i3
  %max = call i3 @llvm.umax.i3(i3 %ta, i3 %tb)
  %r = zext i3 %max to i8
  ret i8 %r
}

; CHECK-LABEL: define i8 @f4
; CHECK: %1 = and i8 %a, 7
; CHECK: %2 = and i8 %b, 7
; CHECK: [[AZEXT:%.*]] = and i8 %1, 7
; CHECK: [[BZEXT:%.*]] = and i8 %2, 7
; CHECK: [[MAX:%.*]] = call i8 @llvm.umax.i8(i8 [[AZEXT]], i8 [[BZEXT]])
; CHECK: [[R:%.*]] = and i8 [[MAX]], 7
; CHECK: ret i8 [[R]]

; A negative constant operand must stay negative.
define i8 @f5(i8 %a) {
  %ta = trunc i8 %a to i3
  %max = call i3 @llvm.smax.i3(i3 %ta, i3 -2)
  %r = sext i3 %max to i8
  ret i8 %r
}

; CHECK-LABEL: define i8 @f5
; CHECK: %1 = and i8 %a, 7
; CHECK: [[ASHL:%.*]] = shl i8 %1, 5
; CHECK: [[ASEXT:%.*]] = ashr i8 [[ASHL]], 5
; CHECK: [[MAX:%.*]] = call i8 @llvm.smax.i8(i8 [[ASEXT]], i8 -2)

; An operand whose promoted value has bits set above its original width.
define i8 @f6(i8 %a, i8 %b) {
  %ta = trunc i8 %a to i3
  %tb = trunc i8 %b to i3
  %s = add i3 %ta, %tb
  %min = call i3 @llvm.umin.i3(i3 %s, i3 3)
  %r = zext i3 %min to i8
  ret i8 %r
}

; CHECK-LABEL: define i8 @f6
; CHECK: %s.promote = add i8 %1, %2
; CHECK: [[SZEXT:%.*]] = and i8 %s.promote, 7
; CHECK: [[MIN:%.*]] = call i8 @llvm.umin.i8(i8 [[SZEXT]], i8 3)

define <2 x i8> @test_vector_smin(<2 x i8> %a) {
  %ta = trunc <2 x i8> %a to <2 x i3>
  %min = call <2 x i3> @llvm.smin.v2i3(<2 x i3> %ta, <2 x i3> <i3 -2, i3 1>)
  %r = zext <2 x i3> %min to <2 x i8>
  ret <2 x i8> %r
}

; CHECK-LABEL: define <2 x i8> @test_vector_smin
; CHECK-PRE22: [[A:%.*]] = and <2 x i8> %a, <i8 7, i8 7>
; CHECK-LLVM22: [[A:%.*]] = and <2 x i8> %a, splat (i8 7)
; CHECK-PRE22: [[ASHL:%.*]] = shl <2 x i8> [[A]], <i8 5, i8 5>
; CHECK-LLVM22: [[ASHL:%.*]] = shl <2 x i8> [[A]], splat (i8 5)
; CHECK-PRE22: [[ASEXT:%.*]] = ashr <2 x i8> [[ASHL]], <i8 5, i8 5>
; CHECK-LLVM22: [[ASEXT:%.*]] = ashr <2 x i8> [[ASHL]], splat (i8 5)
; CHECK: [[MIN:%.*]] = call <2 x i8> @llvm.smin.v2i8(<2 x i8> [[ASEXT]], <2 x i8> <i8 -2, i8 1>)
; CHECK-PRE22: [[R:%.*]] = and <2 x i8> [[MIN]], <i8 7, i8 7>
; CHECK-LLVM22: [[R:%.*]] = and <2 x i8> [[MIN]], splat (i8 7)
; CHECK: ret <2 x i8> [[R]]

define <2 x i8> @test_vector_umax(<2 x i8> %a) {
  %ta = trunc <2 x i8> %a to <2 x i3>
  %s = add <2 x i3> %ta, <i3 3, i3 5>
  %max = call <2 x i3> @llvm.umax.v2i3(<2 x i3> %s, <2 x i3> <i3 3, i3 5>)
  %r = zext <2 x i3> %max to <2 x i8>
  ret <2 x i8> %r
}

; CHECK-LABEL: define <2 x i8> @test_vector_umax
; CHECK-PRE22: [[A:%.*]] = and <2 x i8> %a, <i8 7, i8 7>
; CHECK-LLVM22: [[A:%.*]] = and <2 x i8> %a, splat (i8 7)
; CHECK: [[S:%.*]] = add <2 x i8> [[A]], <i8 3, i8 5>
; CHECK-PRE22: [[SZEXT:%.*]] = and <2 x i8> [[S]], <i8 7, i8 7>
; CHECK-LLVM22: [[SZEXT:%.*]] = and <2 x i8> [[S]], splat (i8 7)
; CHECK: [[MAX:%.*]] = call <2 x i8> @llvm.umax.v2i8(<2 x i8> [[SZEXT]], <2 x i8> <i8 3, i8 5>)
; CHECK-PRE22: [[R:%.*]] = and <2 x i8> [[MAX]], <i8 7, i8 7>
; CHECK-LLVM22: [[R:%.*]] = and <2 x i8> [[MAX]], splat (i8 7)
; CHECK: ret <2 x i8> [[R]]

declare i40 @llvm.smin.i40(i40, i40)
declare i31 @llvm.smax.i31(i31, i31)
declare i9  @llvm.umin.i9(i9, i9)
declare i3  @llvm.umax.i3(i3, i3)
declare i3  @llvm.smax.i3(i3, i3)
declare i3  @llvm.umin.i3(i3, i3)
declare <2 x i3> @llvm.smin.v2i3(<2 x i3>, <2 x i3>)
declare <2 x i3> @llvm.umax.v2i3(<2 x i3>, <2 x i3>)
