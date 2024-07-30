;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

define i8 @A0(i8, i8, i8) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i8 %2, 7
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i8 8, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i8 %0, [[AND]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i8 %1, [[SUB]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i8 [[SHL]], [[LSHR]]
; CHECK:  ret i8 [[OR]]
  %3 = call i8 @llvm.fshl.i8(i8 %0, i8 %1, i8 %2)
  ret i8 %3
}

define i8 @A1(i8, i8) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i8 %0, 0
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i8 %1, 8
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i8 [[SHL]], [[LSHR]]
; CHECK:  ret i8 [[OR]]
  %2 = call i8 @llvm.fshl.i8(i8 %0, i8 %1, i8 8)
  ret i8 %2
}

define i32 @A2(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 1
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 31
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  ret i32 [[OR]]
  %2 = call i32 @llvm.fshl.i32(i32 %0, i32 %1, i32 33)
  ret i32 %2
}

define i32 @A3(i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 5
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %0, 27
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  ret i32 [[OR]]
  %1 = call i32 @llvm.fshl.i32(i32 %0, i32 %0, i32 5)
  ret i32 %1
}

define i64 @A4(i64, i64, i64) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i64 %2, 63
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i64 64, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i64 %0, [[AND]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i64 %1, [[SUB]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i64 [[SHL]], [[LSHR]]
; CHECK:  ret i64 [[OR]]
  %3 = call i64 @llvm.fshl.i64(i64 %0, i64 %1, i64 %2)
  ret i64 %3
}

define i64 @A5(i64, i64) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i64 %0, 36
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i64 %1, 28
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i64 [[SHL]], [[LSHR]]
; CHECK:  ret i64 [[OR]]
  %2 = call i64 @llvm.fshl.i64(i64 %0, i64 %1, i64 100)
  ret i64 %2
}

define <2 x i16> @A6(<2 x i16>, <2 x i16>, <2 x i16>) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and <2 x i16> %2, <i16 15, i16 15>
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub <2 x i16> <i16 16, i16 16>, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl <2 x i16> %0, [[AND]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr <2 x i16> %1, [[SUB]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or <2 x i16> [[SHL]], [[LSHR]]
; CHECK:  ret <2 x i16> [[OR]]
  %3 = call <2 x i16> @llvm.fshl.v2i16(<2 x i16> %0, <2 x i16> %1, <2 x i16> %2)
  ret <2 x i16> %3
}

define i8 @B0(i8, i8, i8) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i8 %2, 7
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i8 8, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i8 %0, [[SUB]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i8 %1, [[AND]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i8 [[SHL]], [[LSHR]]
; CHECK:  ret i8 [[OR]]
  %3 = call i8 @llvm.fshr.i8(i8 %0, i8 %1, i8 %2)
  ret i8 %3
}

define i8 @B1(i8, i8) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i8 %0, 8
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i8 %1, 0
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i8 [[SHL]], [[LSHR]]
; CHECK:  ret i8 [[OR]]
  %2 = call i8 @llvm.fshr.i8(i8 %0, i8 %1, i8 8)
  ret i8 %2
}

define i32 @B2(i32, i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 31
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %1, 1
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  ret i32 [[OR]]
  %2 = call i32 @llvm.fshr.i32(i32 %0, i32 %1, i32 33)
  ret i32 %2
}

define i32 @B3(i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i32 %0, 27
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i32 %0, 5
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i32 [[SHL]], [[LSHR]]
; CHECK:  ret i32 [[OR]]
  %1 = call i32 @llvm.fshr.i32(i32 %0, i32 %0, i32 5)
  ret i32 %1
}

define i64 @B4(i64, i64, i64) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and i64 %2, 63
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub i64 64, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i64 %0, [[SUB]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i64 %1, [[AND]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i64 [[SHL]], [[LSHR]]
; CHECK:  ret i64 [[OR]]
  %3 = call i64 @llvm.fshr.i64(i64 %0, i64 %1, i64 %2)
  ret i64 %3
}

define i64 @B5(i64, i64) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl i64 %0, 28
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr i64 %1, 36
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or i64 [[SHL]], [[LSHR]]
; CHECK:  ret i64 [[OR]]
  %2 = call i64 @llvm.fshr.i64(i64 %0, i64 %1, i64 100)
  ret i64 %2
}

define <2 x i16> @B6(<2 x i16>, <2 x i16>, <2 x i16>) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[AND:%[a-zA-Z0-9]+]] = and <2 x i16> %2, <i16 15, i16 15>
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = sub <2 x i16> <i16 16, i16 16>, [[AND]]
; CHECK:  [[SHL:%[a-zA-Z0-9]+]] = shl <2 x i16> %0, [[SUB]]
; CHECK:  [[LSHR:%[a-zA-Z0-9]+]] = lshr <2 x i16> %1, [[AND]]
; CHECK:  [[OR:%[a-zA-Z0-9]+]] = or <2 x i16> [[SHL]], [[LSHR]]
; CHECK:  ret <2 x i16> [[OR]]
  %3 = call <2 x i16> @llvm.fshr.v2i16(<2 x i16> %0, <2 x i16> %1, <2 x i16> %2)
  ret <2 x i16> %3
}

declare i8 @llvm.fshl.i8(i8, i8, i8)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i64 @llvm.fshl.i64(i64, i64, i64)
declare <2 x i16> @llvm.fshl.v2i16(<2 x i16>, <2 x i16>, <2 x i16>)

declare i8 @llvm.fshr.i8(i8, i8, i8)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare i64 @llvm.fshr.i64(i64, i64, i64)
declare <2 x i16> @llvm.fshr.v2i16(<2 x i16>, <2 x i16>, <2 x i16>)
