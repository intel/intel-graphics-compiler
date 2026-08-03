;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-22-plus
; RUN: igc_opt -igc-replace-unsupported-intrinsics -S %s 2>&1 | FileCheck %s

define i32 @scmp_i32_i32(i32 %src0, i32 %src1) {
  ; CHECK-LABEL: define i32 @scmp_i32_i32(
  ; CHECK: [[LT:%.*]] = icmp slt i32 %src0, %src1
  ; CHECK: [[GT:%.*]] = icmp sgt i32 %src0, %src1
  ; CHECK: [[GT_SEL:%.*]] = select i1 [[GT]], i32 1, i32 0
  ; CHECK: [[R:%.*]] = select i1 [[LT]], i32 -1, i32 [[GT_SEL]]
  ; CHECK: ret i32 [[R]]
  %1 = call i32 @llvm.scmp.i32.i32(i32 %src0, i32 %src1)
  ret i32 %1
}

define i32 @scmp_i32_i64(i64 %src0, i64 %src1) {
  ; CHECK-LABEL: define i32 @scmp_i32_i64(
  ; CHECK: [[LT:%.*]] = icmp slt i64 %src0, %src1
  ; CHECK: [[GT:%.*]] = icmp sgt i64 %src0, %src1
  ; CHECK: [[GT_SEL:%.*]] = select i1 [[GT]], i32 1, i32 0
  ; CHECK: [[R:%.*]] = select i1 [[LT]], i32 -1, i32 [[GT_SEL]]
  ; CHECK: ret i32 [[R]]
  %1 = call i32 @llvm.scmp.i32.i64(i64 %src0, i64 %src1)
  ret i32 %1
}

define <2 x i32> @scmp_v2i32_v2i32(<2 x i32> %src0, <2 x i32> %src1) {
  ; CHECK-LABEL: define <2 x i32> @scmp_v2i32_v2i32(
  ; CHECK: [[VAL0:%.*]] = extractelement <2 x i32> %src0, i32 0
  ; CHECK: [[VAL1:%.*]] = extractelement <2 x i32> %src1, i32 0
  ; CHECK: [[LT0:%.*]] = icmp slt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT0:%.*]] = icmp sgt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT_SEL0:%.*]] = select i1 [[GT0]], i32 1, i32 0
  ; CHECK: [[R0:%.*]] = select i1 [[LT0]], i32 -1, i32 [[GT_SEL0]]
  ; CHECK: [[RES0:%.*]] = insertelement <2 x i32> poison, i32 [[R0]], i32 0
  ; CHECK: [[VAL2:%.*]] = extractelement <2 x i32> %src0, i32 1
  ; CHECK: [[VAL3:%.*]] = extractelement <2 x i32> %src1, i32 1
  ; CHECK: [[LT1:%.*]] = icmp slt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT1:%.*]] = icmp sgt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT_SEL1:%.*]] = select i1 [[GT1]], i32 1, i32 0
  ; CHECK: [[R1:%.*]] = select i1 [[LT1]], i32 -1, i32 [[GT_SEL1]]
  ; CHECK: [[RES1:%.*]] = insertelement <2 x i32> [[RES0]], i32 [[R1]], i32 1
  ; CHECK: ret <2 x i32> [[RES1]]
  %1 = call <2 x i32> @llvm.scmp.v2i32.v2i32(<2 x i32> %src0, <2 x i32> %src1)
  ret <2 x i32> %1
}

define <2 x i8> @scmp_v2i8_v2i32(<2 x i32> %src0, <2 x i32> %src1) {
  ; CHECK-LABEL: define <2 x i8> @scmp_v2i8_v2i32(
  ; CHECK: [[VAL0:%.*]] = extractelement <2 x i32> %src0, i32 0
  ; CHECK: [[VAL1:%.*]] = extractelement <2 x i32> %src1, i32 0
  ; CHECK: [[LT0:%.*]] = icmp slt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT0:%.*]] = icmp sgt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT_SEL0:%.*]] = select i1 [[GT0]], i8 1, i8 0
  ; CHECK: [[R0:%.*]] = select i1 [[LT0]], i8 -1, i8 [[GT_SEL0]]
  ; CHECK: [[RES0:%.*]] = insertelement <2 x i8> poison, i8 [[R0]], i32 0
  ; CHECK: [[VAL2:%.*]] = extractelement <2 x i32> %src0, i32 1
  ; CHECK: [[VAL3:%.*]] = extractelement <2 x i32> %src1, i32 1
  ; CHECK: [[LT1:%.*]] = icmp slt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT1:%.*]] = icmp sgt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT_SEL1:%.*]] = select i1 [[GT1]], i8 1, i8 0
  ; CHECK: [[R1:%.*]] = select i1 [[LT1]], i8 -1, i8 [[GT_SEL1]]
  ; CHECK: [[RES1:%.*]] = insertelement <2 x i8> [[RES0]], i8 [[R1]], i32 1
  ; CHECK: ret <2 x i8> [[RES1]]
  %1 = call <2 x i8> @llvm.scmp.v2i8.v2i32(<2 x i32> %src0, <2 x i32> %src1)
  ret <2 x i8> %1
}

define i32 @ucmp_i32_i32(i32 %src0, i32 %src1) {
  ; CHECK-LABEL: define i32 @ucmp_i32_i32(
  ; CHECK: [[LT:%.*]] = icmp ult i32 %src0, %src1
  ; CHECK: [[GT:%.*]] = icmp ugt i32 %src0, %src1
  ; CHECK: [[GT_SEL:%.*]] = select i1 [[GT]], i32 1, i32 0
  ; CHECK: [[R:%.*]] = select i1 [[LT]], i32 -1, i32 [[GT_SEL]]
  ; CHECK: ret i32 [[R]]
  %1 = call i32 @llvm.ucmp.i32.i32(i32 %src0, i32 %src1)
  ret i32 %1
}

define i32 @ucmp_i32_i64(i64 %src0, i64 %src1) {
  ; CHECK-LABEL: define i32 @ucmp_i32_i64(
  ; CHECK: [[LT:%.*]] = icmp ult i64 %src0, %src1
  ; CHECK: [[GT:%.*]] = icmp ugt i64 %src0, %src1
  ; CHECK: [[GT_SEL:%.*]] = select i1 [[GT]], i32 1, i32 0
  ; CHECK: [[R:%.*]] = select i1 [[LT]], i32 -1, i32 [[GT_SEL]]
  ; CHECK: ret i32 [[R]]
  %1 = call i32 @llvm.ucmp.i32.i64(i64 %src0, i64 %src1)
  ret i32 %1
}

define <2 x i32> @ucmp_v2i32_v2i32(<2 x i32> %src0, <2 x i32> %src1) {
  ; CHECK-LABEL: define <2 x i32> @ucmp_v2i32_v2i32(
  ; CHECK: [[VAL0:%.*]] = extractelement <2 x i32> %src0, i32 0
  ; CHECK: [[VAL1:%.*]] = extractelement <2 x i32> %src1, i32 0
  ; CHECK: [[LT0:%.*]] = icmp ult i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT0:%.*]] = icmp ugt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT_SEL0:%.*]] = select i1 [[GT0]], i32 1, i32 0
  ; CHECK: [[R0:%.*]] = select i1 [[LT0]], i32 -1, i32 [[GT_SEL0]]
  ; CHECK: [[RES0:%.*]] = insertelement <2 x i32> poison, i32 [[R0]], i32 0
  ; CHECK: [[VAL2:%.*]] = extractelement <2 x i32> %src0, i32 1
  ; CHECK: [[VAL3:%.*]] = extractelement <2 x i32> %src1, i32 1
  ; CHECK: [[LT1:%.*]] = icmp ult i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT1:%.*]] = icmp ugt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT_SEL1:%.*]] = select i1 [[GT1]], i32 1, i32 0
  ; CHECK: [[R1:%.*]] = select i1 [[LT1]], i32 -1, i32 [[GT_SEL1]]
  ; CHECK: [[RES1:%.*]] = insertelement <2 x i32> [[RES0]], i32 [[R1]], i32 1
  ; CHECK: ret <2 x i32> [[RES1]]
  %1 = call <2 x i32> @llvm.ucmp.v2i32.v2i32(<2 x i32> %src0, <2 x i32> %src1)
  ret <2 x i32> %1
}

define <2 x i8> @ucmp_v2i8_v2i32(<2 x i32> %src0, <2 x i32> %src1) {
  ; CHECK-LABEL: define <2 x i8> @ucmp_v2i8_v2i32(
  ; CHECK: [[VAL0:%.*]] = extractelement <2 x i32> %src0, i32 0
  ; CHECK: [[VAL1:%.*]] = extractelement <2 x i32> %src1, i32 0
  ; CHECK: [[LT0:%.*]] = icmp ult i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT0:%.*]] = icmp ugt i32 [[VAL0]], [[VAL1]]
  ; CHECK: [[GT_SEL0:%.*]] = select i1 [[GT0]], i8 1, i8 0
  ; CHECK: [[R0:%.*]] = select i1 [[LT0]], i8 -1, i8 [[GT_SEL0]]
  ; CHECK: [[RES0:%.*]] = insertelement <2 x i8> poison, i8 [[R0]], i32 0
  ; CHECK: [[VAL2:%.*]] = extractelement <2 x i32> %src0, i32 1
  ; CHECK: [[VAL3:%.*]] = extractelement <2 x i32> %src1, i32 1
  ; CHECK: [[LT1:%.*]] = icmp ult i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT1:%.*]] = icmp ugt i32 [[VAL2]], [[VAL3]]
  ; CHECK: [[GT_SEL1:%.*]] = select i1 [[GT1]], i8 1, i8 0
  ; CHECK: [[R1:%.*]] = select i1 [[LT1]], i8 -1, i8 [[GT_SEL1]]
  ; CHECK: [[RES1:%.*]] = insertelement <2 x i8> [[RES0]], i8 [[R1]], i32 1
  ; CHECK: ret <2 x i8> [[RES1]]
  %1 = call <2 x i8> @llvm.ucmp.v2i8.v2i32(<2 x i32> %src0, <2 x i32> %src1)
  ret <2 x i8> %1
}
