;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s --implicit-check-not='call i64 @llvm.ctpop.i64(' --implicit-check-not='call <2 x i64> @llvm.ctpop.v2i64(' < %t

; i32 is supported natively and left alone
define i32 @ctpop_i32(i32 %x) {
entry:
; CHECK-LABEL: @ctpop_i32(
; CHECK:  [[POP:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 %x)
; CHECK:  ret i32 [[POP]]
  %0 = call i32 @llvm.ctpop.i32(i32 %x)
  ret i32 %0
}

; i64 is split into lo/hi halves
define i64 @ctpop_i64(i64 %x) {
entry:
; CHECK-LABEL: @ctpop_i64(
; CHECK:  [[LO:%[a-zA-Z0-9.]+]] = trunc i64 %x to i32
; CHECK:  [[SHR:%[a-zA-Z0-9.]+]] = lshr i64 %x, 32
; CHECK:  [[HI:%[a-zA-Z0-9.]+]] = trunc i64 [[SHR]] to i32
; CHECK:  [[POP_LO:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[LO]])
; CHECK:  [[POP_HI:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[HI]])
; CHECK:  [[ADD:%[a-zA-Z0-9.]+]] = add nuw nsw i32 [[POP_LO]], [[POP_HI]]
; CHECK:  [[ZE:%[a-zA-Z0-9.]+]] = zext i32 [[ADD]] to i64
; CHECK:  ret i64 [[ZE]]
  %0 = call i64 @llvm.ctpop.i64(i64 %x)
  ret i64 %0
}

; what InstCombine makes of (x & (x - 1)) == 0
define i1 @ispow2_i64(i64 %x) {
entry:
; CHECK-LABEL: @ispow2_i64(
; CHECK:  [[POP_LO:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32
; CHECK:  [[POP_HI:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32
; CHECK:  [[ADD:%[a-zA-Z0-9.]+]] = add nuw nsw i32 [[POP_LO]], [[POP_HI]]
; CHECK:  [[ZE:%[a-zA-Z0-9.]+]] = zext i32 [[ADD]] to i64
; CHECK:  [[CMP:%[a-zA-Z0-9.]+]] = icmp ult i64 [[ZE]], 2
; CHECK:  ret i1 [[CMP]]
  %0 = call i64 @llvm.ctpop.i64(i64 %x)
  %1 = icmp ult i64 %0, 2
  ret i1 %1
}

; vectors of i64 are split per element
define <2 x i64> @ctpop_v2i64(<2 x i64> %x) {
entry:
; CHECK-LABEL: @ctpop_v2i64(
; CHECK:  [[X0:%[a-zA-Z0-9.]+]] = extractelement <2 x i64> %x, i64 0
; CHECK:  [[LO0:%[a-zA-Z0-9.]+]] = trunc i64 [[X0]] to i32
; CHECK:  [[SHR0:%[a-zA-Z0-9.]+]] = lshr i64 [[X0]], 32
; CHECK:  [[HI0:%[a-zA-Z0-9.]+]] = trunc i64 [[SHR0]] to i32
; CHECK:  [[POP_LO0:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[LO0]])
; CHECK:  [[POP_HI0:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[HI0]])
; CHECK:  [[ADD0:%[a-zA-Z0-9.]+]] = add nuw nsw i32 [[POP_LO0]], [[POP_HI0]]
; CHECK:  [[ZE0:%[a-zA-Z0-9.]+]] = zext i32 [[ADD0]] to i64
; CHECK:  [[V0:%[a-zA-Z0-9.]+]] = insertelement <2 x i64> poison, i64 [[ZE0]], i32 0
; CHECK:  [[X1:%[a-zA-Z0-9.]+]] = extractelement <2 x i64> %x, i64 1
; CHECK:  [[LO1:%[a-zA-Z0-9.]+]] = trunc i64 [[X1]] to i32
; CHECK:  [[SHR1:%[a-zA-Z0-9.]+]] = lshr i64 [[X1]], 32
; CHECK:  [[HI1:%[a-zA-Z0-9.]+]] = trunc i64 [[SHR1]] to i32
; CHECK:  [[POP_LO1:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[LO1]])
; CHECK:  [[POP_HI1:%[a-zA-Z0-9.]+]] = call i32 @llvm.ctpop.i32(i32 [[HI1]])
; CHECK:  [[ADD1:%[a-zA-Z0-9.]+]] = add nuw nsw i32 [[POP_LO1]], [[POP_HI1]]
; CHECK:  [[ZE1:%[a-zA-Z0-9.]+]] = zext i32 [[ADD1]] to i64
; CHECK:  [[V1:%[a-zA-Z0-9.]+]] = insertelement <2 x i64> [[V0]], i64 [[ZE1]], i32 1
; CHECK:  ret <2 x i64> [[V1]]
  %0 = call <2 x i64> @llvm.ctpop.v2i64(<2 x i64> %x)
  ret <2 x i64> %0
}

declare i32 @llvm.ctpop.i32(i32)
declare i64 @llvm.ctpop.i64(i64)
declare <2 x i64> @llvm.ctpop.v2i64(<2 x i64>)
