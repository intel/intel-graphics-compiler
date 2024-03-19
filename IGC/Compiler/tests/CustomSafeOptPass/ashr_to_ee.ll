;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-gen-specific-pattern --verify --platformdg2 -S %s | FileCheck %s

define spir_kernel void @testkernel_16(i32 %x) {
; CHECK-LABEL: @testkernel_16(
; CHECK:       entry:
; CHECK:         [[SHL:%.*]] = shl i32 [[X:%.*]], 16
; CHECK:         [[TMP0:%.*]] = bitcast i32 [[X]] to <2 x i16>
; CHECK:         [[TMP1:%.*]] = extractelement <2 x i16> [[TMP0]], i32 0
; CHECK:         [[TMP2:%.*]] = sext i16 [[TMP1]] to i32
; CHECK:         [[TMP3:%.*]] = bitcast i32 [[X]] to <2 x i16>
; CHECK:         [[TMP4:%.*]] = extractelement <2 x i16> [[TMP3]], i32 1
; CHECK:         [[TMP5:%.*]] = sext i16 [[TMP4]] to i32
; CHECK:         [[RES:%.*]] = add i32 [[TMP2]], [[TMP5]]
; CHECK:         ret void
;

entry:
  %Shl = shl i32 %x, 16
  %Lo = ashr exact i32 %Shl, 16
  %Hi = ashr i32 %x, 16
  %Res = add i32 %Lo, %Hi
  ret void
}

define spir_kernel void @testkernel_8(i32 %x, <4 x i32> %y) {
; CHECK-LABEL: @testkernel_8(
; CHECK:       entry:

; Not changed part of the code - listed common pattern just for bigger picture

; CHECK:         [[ASTYPE:%.*]] = bitcast i32 [[X:%.*]] to <4 x i8>
; CHECK:         [[ASTYPE_SCALAR111:%.*]] = extractelement <4 x i8> [[ASTYPE]], i64 1
; CHECK:         [[ASTYPE_SCALAR112:%.*]] = extractelement <4 x i8> [[ASTYPE]], i64 2
; CHECK:         [[ASTYPE_SCALAR113:%.*]] = extractelement <4 x i8> [[ASTYPE]], i64 3

; This is not used anymore
; CHECK:         [[SEXT:%.*]] = shl i32 [[X]], 24

; i8 extract element is added
; CHECK:         [[TMP0:%.*]] = bitcast i32 [[X]] to <4 x i8>
; CHECK:         [[TMP1:%.*]] = extractelement <4 x i8> [[TMP0]], i32 0
; CHECK:         [[CONV1:%.*]] = sext i8 [[TMP1]] to i32

; not changed
; CHECK:         [[SCALAR1:%.*]] = extractelement <4 x i32> [[Y:%.*]], i64 0

; Ensure EE is used (Conv1)
; CHECK:         [[ADD_I1:%.*]] = add nsw i32 [[SCALAR1]], [[CONV1]]
; CHECK:         [[SCALAR2:%.*]] = extractelement <4 x i32> [[Y]], i64 1

; The rest of the code, listed common pattern for bigger picture
; CHECK:         [[CONV2:%.*]] = sext i8 [[ASTYPE_SCALAR111]] to i32
; CHECK:         [[ADD_I2:%.*]] = add nsw i32 [[SCALAR2]], [[CONV2]]
; CHECK:         [[SCALAR3:%.*]] = extractelement <4 x i32> [[Y]], i64 2
; CHECK:         [[CONV3:%.*]] = sext i8 [[ASTYPE_SCALAR112]] to i32
; CHECK:         [[ADD_I3:%.*]] = add nsw i32 [[SCALAR3]], [[CONV3]]
; CHECK:         [[SCALAR4:%.*]] = extractelement <4 x i32> [[Y]], i64 3
; CHECK:         [[CONV4:%.*]] = sext i8 [[ASTYPE_SCALAR113]] to i32
; CHECK:         [[ADD_I4:%.*]] = add nsw i32 [[SCALAR4]], [[CONV4]]
; CHECK:         ret void
;
entry:
  %astype = bitcast i32 %x to <4 x i8>
  %astype.scalar111 = extractelement <4 x i8> %astype, i64 1
  %astype.scalar112 = extractelement <4 x i8> %astype, i64 2
  %astype.scalar113 = extractelement <4 x i8> %astype, i64 3

  ; The transformation applies to this part
  ; ASHR will be changed to EE
  %sext = shl i32 %x, 24
  %conv1 = ashr exact i32 %sext, 24

  %scalar1 = extractelement <4 x i32> %y, i64 0
  %add.i1 = add nsw i32 %scalar1, %conv1

  %scalar2 = extractelement <4 x i32> %y, i64 1
  %conv2 = sext i8 %astype.scalar111 to i32
  %add.i2 = add nsw i32 %scalar2, %conv2

  %scalar3 = extractelement <4 x i32> %y, i64 2
  %conv3 = sext i8 %astype.scalar112 to i32
  %add.i3 = add nsw i32 %scalar3, %conv3

  %scalar4 = extractelement <4 x i32> %y, i64 3
  %conv4 = sext i8 %astype.scalar113 to i32
  %add.i4 = add nsw i32 %scalar4, %conv4

  ret void
}

