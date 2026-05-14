;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; i8, zero is defined (cttz(0) = 8)
define i8 @cttz_i8(i8 %x) {
entry:
; CHECK-LABEL: @cttz_i8(
; CHECK:  [[ZEXT:%[a-zA-Z0-9.]+]] = zext i8 %x to i32
; CHECK:  [[FBL:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[ZEXT]])
; CHECK:  [[CMP:%[a-zA-Z0-9.]+]] = icmp eq i8 %x, 0
; CHECK:  [[SEL:%[a-zA-Z0-9.]+]] = select i1 [[CMP]], i32 8, i32 [[FBL]]
; CHECK:  [[TR:%[a-zA-Z0-9.]+]] = trunc i32 [[SEL]] to i8
; CHECK:  ret i8 [[TR]]
  %0 = call i8 @llvm.cttz.i8(i8 %x, i1 false)
  ret i8 %0
}

; i16, zero is defined (cttz(0) = 16)
define i16 @cttz_i16(i16 %x) {
entry:
; CHECK-LABEL: @cttz_i16(
; CHECK:  [[ZEXT:%[a-zA-Z0-9.]+]] = zext i16 %x to i32
; CHECK:  [[FBL:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[ZEXT]])
; CHECK:  [[CMP:%[a-zA-Z0-9.]+]] = icmp eq i16 %x, 0
; CHECK:  [[SEL:%[a-zA-Z0-9.]+]] = select i1 [[CMP]], i32 16, i32 [[FBL]]
; CHECK:  [[TR:%[a-zA-Z0-9.]+]] = trunc i32 [[SEL]] to i16
; CHECK:  ret i16 [[TR]]
  %0 = call i16 @llvm.cttz.i16(i16 %x, i1 false)
  ret i16 %0
}

; i32, zero is defined (cttz(0) = 32)
define i32 @cttz_i32(i32 %x) {
entry:
; CHECK-LABEL: @cttz_i32(
; CHECK:  [[FBL:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 %x)
; CHECK:  [[CMP:%[a-zA-Z0-9.]+]] = icmp eq i32 %x, 0
; CHECK:  [[SEL:%[a-zA-Z0-9.]+]] = select i1 [[CMP]], i32 32, i32 [[FBL]]
; CHECK:  ret i32 [[SEL]]
  %0 = call i32 @llvm.cttz.i32(i32 %x, i1 false)
  ret i32 %0
}

; i32, zero is poison — no select needed
define i32 @cttz_i32_poison(i32 %x) {
entry:
; CHECK-LABEL: @cttz_i32_poison(
; CHECK:  [[FBL:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 %x)
; CHECK-NOT:  icmp eq
; CHECK:  ret i32 [[FBL]]
  %0 = call i32 @llvm.cttz.i32(i32 %x, i1 true)
  ret i32 %0
}

; i64, zero is defined (cttz(0) = 64) — split into lo/hi halves
define i64 @cttz_i64(i64 %x) {
entry:
; CHECK-LABEL: @cttz_i64(
; CHECK:  [[LO:%[a-zA-Z0-9.]+]] = trunc i64 %x to i32
; CHECK:  [[SHR:%[a-zA-Z0-9.]+]] = lshr i64 %x, 32
; CHECK:  [[HI:%[a-zA-Z0-9.]+]] = trunc i64 [[SHR]] to i32
; CHECK:  [[FBL_LO:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[LO]])
; CHECK:  [[FBL_HI:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[HI]])
; CHECK:  [[LOZ:%[a-zA-Z0-9.]+]] = icmp eq i32 [[LO]], 0
; CHECK:  [[ADD:%[a-zA-Z0-9.]+]] = add i32 [[FBL_HI]], 32
; CHECK:  [[SEL1:%[a-zA-Z0-9.]+]] = select i1 [[LOZ]], i32 [[ADD]], i32 [[FBL_LO]]
; CHECK:  [[XZ:%[a-zA-Z0-9.]+]] = icmp eq i64 %x, 0
; CHECK:  [[SEL2:%[a-zA-Z0-9.]+]] = select i1 [[XZ]], i32 64, i32 [[SEL1]]
; CHECK:  [[ZE:%[a-zA-Z0-9.]+]] = zext i32 [[SEL2]] to i64
; CHECK:  ret i64 [[ZE]]
  %0 = call i64 @llvm.cttz.i64(i64 %x, i1 false)
  ret i64 %0
}

; Vector cttz — verifies the per-element loop
define <2 x i32> @cttz_v2i32(<2 x i32> %x) {
entry:
; CHECK-LABEL: @cttz_v2i32(
; CHECK:  extractelement <2 x i32> %x
; CHECK:  call i32 @llvm.genx.GenISA.firstbitLo(i32
; CHECK:  insertelement <2 x i32>
; CHECK:  extractelement <2 x i32> %x
; CHECK:  call i32 @llvm.genx.GenISA.firstbitLo(i32
; CHECK:  insertelement <2 x i32>
; CHECK:  ret <2 x i32>
  %0 = call <2 x i32> @llvm.cttz.v2i32(<2 x i32> %x, i1 false)
  ret <2 x i32> %0
}

declare i8 @llvm.cttz.i8(i8, i1 immarg)
declare i16 @llvm.cttz.i16(i16, i1 immarg)
declare i32 @llvm.cttz.i32(i32, i1 immarg)
declare i64 @llvm.cttz.i64(i64, i1 immarg)
declare <2 x i32> @llvm.cttz.v2i32(<2 x i32>, i1 immarg)
