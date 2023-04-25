;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

define i8 @A0(i8) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[CONV_0:%[a-zA-Z0-9]+]] = zext i8 %0 to i32
; CHECK:  [[CALL:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0]], i1 false)
; CHECK:  [[CONV_1:%[a-zA-Z0-9]+]] = trunc i32 [[CALL]] to i8
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = add nsw i8 [[CONV_1]], -24
; CHECK:  ret i8 [[SUB]]
  %1 = call i8 @llvm.ctlz.i8(i8 %0, i1 false)
  ret i8 %1
}


define i16 @A1(i16) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[CONV_0:%[a-zA-Z0-9]+]] = zext i16 %0 to i32
; CHECK:  [[CALL:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0]], i1 false)
; CHECK:  [[CONV_1:%[a-zA-Z0-9]+]] = trunc i32 [[CALL]] to i16
; CHECK:  [[SUB:%[a-zA-Z0-9]+]] = add nsw i16 [[CONV_1]], -16
; CHECK:  ret i16 [[SUB]]
  %1 = call i16 @llvm.ctlz.i16(i16 %0, i1 false)
  ret i16 %1
}


define i32 @A2(i32) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[CALL:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 %0, i1 false)
; CHECK:  ret i32 [[CALL]]
  %1 = call i32 @llvm.ctlz.i32(i32 %0, i1 false)
  ret i32 %1
}

define <2 x i8> @A3(<2 x i8>) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[EXTRACT_0:%[a-zA-Z0-9]+]] = extractelement <2 x i8> %0, [[INDEX_TYPE_E:i(16|32|64)]] 0
; CHECK:  [[CONV_0_0:%[a-zA-Z0-9]+]] = zext i8 [[EXTRACT_0]] to i32
; CHECK:  [[CALL_0:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0_0]], i1 false)
; CHECK:  [[CONV_0_1:%[a-zA-Z0-9]+]] = trunc i32 [[CALL_0]] to i8
; CHECK:  [[SUB_0:%[a-zA-Z0-9]+]] = add nsw i8 [[CONV_0_1]], -24
; CHECK:  [[INSERT_0:%[a-zA-Z0-9]+]] = insertelement <2 x i8> undef, i8 [[SUB_0]], [[INDEX_TYPE_I:i(16|32|64)]] 0

; CHECK:  [[EXTRACT_1:%[a-zA-Z0-9]+]] = extractelement <2 x i8> %0, [[INDEX_TYPE_E]] 1
; CHECK:  [[CONV_1_0:%[a-zA-Z0-9]+]] = zext i8 [[EXTRACT_1]] to i32
; CHECK:  [[CALL_1:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_1_0]], i1 false)
; CHECK:  [[CONV_1_1:%[a-zA-Z0-9]+]] = trunc i32 [[CALL_1]] to i8
; CHECK:  [[SUB_1:%[a-zA-Z0-9]+]] = add nsw i8 [[CONV_1_1]], -24
; CHECK:  [[INSERT_1:%[a-zA-Z0-9]+]] = insertelement <2 x i8> [[INSERT_0]], i8 [[SUB_1]], [[INDEX_TYPE_I]] 1

; CHECK:  ret <2 x i8> [[INSERT_1]]
  %1 = call <2 x i8> @llvm.ctlz.v2i8(<2 x i8> %0, i1 false)
  ret <2 x i8> %1
}


define <2 x i32> @A4(<2 x i32>) {
entry:
; CHECK-LABEL: entry:
; CHECK:  [[EXTRACT_0:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %0, [[INDEX_TYPE_E:i(16|32|64)]] 0
; CHECK:  [[CALL_0:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[EXTRACT_0]], i1 false)
; CHECK:  [[INSERT_0:%[a-zA-Z0-9]+]] = insertelement <2 x i32> undef, i32 [[CALL_0]], [[INDEX_TYPE_I:i(16|32|64)]] 0

; CHECK:  [[EXTRACT_1:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %0, [[INDEX_TYPE_E]] 1
; CHECK:  [[CALL_1:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[EXTRACT_1]], i1 false)
; CHECK:  [[INSERT_1:%[a-zA-Z0-9]+]] = insertelement <2 x i32> [[INSERT_0]], i32 [[CALL_1]], [[INDEX_TYPE_I]] 1

; CHECK:  ret <2 x i32> [[INSERT_1]]
  %1 = call <2 x i32> @llvm.ctlz.v2i32(<2 x i32> %0, i1 false)
  ret <2 x i32> %1
}

define i64 @A5(i64) {
  entry:
; CHECK-LABEL: entry:
; CHECK:  [[CONV_0:%[a-zA-Z0-9]+]] = trunc i64 %0 to i32
; CHECK:  [[CALL_0:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0]], i1 false)
; CHECK:  [[SHR:%[a-zA-Z0-9]+]] = lshr i64 %0, 32
; CHECK:  [[CONV_1:%[a-zA-Z0-9]+]] = trunc i64 [[SHR]] to i32
; CHECK:  [[CALL_1:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_1]], i1 false)
; CHECK:  [[CMP:%[a-zA-Z0-9]+]] = icmp ult i64 %0, 4294967296
; CHECK:  [[ADD:%[a-zA-Z0-9]+]] = add i32 [[CALL_0]], 32
; CHECK:  [[SELECT:%[a-zA-Z0-9]+]] = select i1 [[CMP]], i32 [[ADD]], i32 [[CALL_1]]
; CHECK:  [[CONV_3:%[a-zA-Z0-9]+]] = zext i32 [[SELECT]] to i64
; CHECK:  ret i64 [[CONV_3]]
  %1 = call i64 @llvm.ctlz.i64(i64 %0, i1 false)
  ret i64 %1
}


define <2 x i64> @A6(<2 x i64>) {
  entry:
; CHECK-LABEL: entry:
; CHECK:  [[EXTRACT_0:%[a-zA-Z0-9]+]] = extractelement <2 x i64> %0, [[INDEX_TYPE_E]] 0
; CHECK:  [[CONV_0_0:%[a-zA-Z0-9]+]] = trunc i64 [[EXTRACT_0]] to i32
; CHECK:  [[CALL_0_0:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0_0]], i1 false)
; CHECK:  [[SHR_0:%[a-zA-Z0-9]+]] = lshr i64 [[EXTRACT_0]], 32
; CHECK:  [[CONV_0_1:%[a-zA-Z0-9]+]] = trunc i64 [[SHR_0]] to i32
; CHECK:  [[CALL_1_1:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_0_1]], i1 false)
; CHECK:  [[CMP_0:%[a-zA-Z0-9]+]] = icmp ult i64 [[EXTRACT_0]], 4294967296
; CHECK:  [[ADD_0:%[a-zA-Z0-9]+]] = add i32 [[CALL_0_0]], 32
; CHECK:  [[SELECT_0:%[a-zA-Z0-9]+]] = select i1 [[CMP_0]], i32 [[ADD_0]], i32 [[CALL_1_1]]
; CHECK:  [[CONV_0_3:%[a-zA-Z0-9]+]] = zext i32 [[SELECT_0]] to i64
; CHECK:  [[INSERT_0:%[a-zA-Z0-9]+]] = insertelement <2 x i64> undef, i64 [[CONV_0_3]], [[INDEX_TYPE_I]] 0

; CHECK:  [[EXTRACT_1:%[a-zA-Z0-9]+]] = extractelement <2 x i64> %0, [[INDEX_TYPE_E]] 1
; CHECK:  [[CONV_1_0:%[a-zA-Z0-9]+]] = trunc i64 [[EXTRACT_1]] to i32
; CHECK:  [[CALL_1_0:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_1_0]], i1 false)
; CHECK:  [[SHR_1:%[a-zA-Z0-9]+]] = lshr i64 [[EXTRACT_1]], 32
; CHECK:  [[CONV_1_1:%[a-zA-Z0-9]+]] = trunc i64 [[SHR_1]] to i32
; CHECK:  [[CALL_1_1:%[a-zA-Z0-9]+]] = call i32 @llvm.ctlz.i32(i32 [[CONV_1_1]], i1 false)
; CHECK:  [[CMP_1:%[a-zA-Z0-9]+]] = icmp ult i64 [[EXTRACT_1]], 4294967296
; CHECK:  [[ADD_1:%[a-zA-Z0-9]+]] = add i32 [[CALL_1_0]], 32
; CHECK:  [[SELECT_1:%[a-zA-Z0-9]+]] = select i1 [[CMP_1]], i32 [[ADD_1]], i32 [[CALL_1_1]]
; CHECK:  [[CONV_1_3:%[a-zA-Z0-9]+]] = zext i32 [[SELECT_1]] to i64
; CHECK:  [[INSERT_1:%[a-zA-Z0-9]+]] = insertelement <2 x i64> [[INSERT_0]], i64 [[CONV_1_3]], [[INDEX_TYPE_I]] 1

; CHECK:  ret <2 x i64> [[INSERT_1]]
  %1 = call <2 x i64> @llvm.ctlz.v2i64(<2 x i64> %0, i1 false)
  ret <2 x i64> %1
}



declare i8 @llvm.ctlz.i8(i8, i1)
declare i16 @llvm.ctlz.i16(i16, i1)
declare i32 @llvm.ctlz.i32(i32, i1)
declare i64 @llvm.ctlz.i64(i64, i1)
declare <2 x i8> @llvm.ctlz.v2i8(<2 x i8>, i1)
declare <2 x i32> @llvm.ctlz.v2i32(<2 x i32>, i1)
declare <2 x i64> @llvm.ctlz.v2i64(<2 x i64>, i1)
