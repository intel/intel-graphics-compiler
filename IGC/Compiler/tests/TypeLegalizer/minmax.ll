;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-type-legalizer -verify -S %s -o - | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

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
; CHECK: %3 = call i64 @llvm.smin.i64(i64 %1, i64 %2)
; CHECK: %4 = shl i64 %3, 24
; CHECK: %5 = ashr i64 %4, 24
; CHECK: ret i64 %5

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
; CHECK: %3 = call i32 @llvm.smax.i32(i32 %1, i32 %2)
; CHECK: %4 = shl i32 %3, 1
; CHECK: %5 = ashr i32 %4, 1
; CHECK: ret i32 %5

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
; CHECK: %3 = call i16 @llvm.umin.i16(i16 %1, i16 %2)
; CHECK: ret i16 %3


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
; CHECK: %3 = call i8 @llvm.umax.i8(i8 %1, i8 %2)
; CHECK: ret i8 %3


declare i40 @llvm.smin.i40(i40, i40)
declare i31 @llvm.smax.i31(i31, i31)
declare i9  @llvm.umin.i9(i9, i9)
declare i3  @llvm.umax.i3(i3, i3)
