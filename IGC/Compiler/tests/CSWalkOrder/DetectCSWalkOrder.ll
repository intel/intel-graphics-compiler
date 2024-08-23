;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt -igc-DetectCSWalkOrder -inputcs -regkey EnableDetectCSWalkOrder -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DetectCSWalkOrder
; ------------------------------------------------

define void @main(<8 x i32> %r0, i8* %privateBase) {
; CHECK:    %f5 = lshr i16 %f4, 1
; CHECK:    %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
; CHECK:    %LocalID_Z = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 19)
; CHECK:    %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK:    [[TMP1:%.*]] = shl i32 %LocalID_Y, 4
; CHECK:    [[TMP2:%.*]] = shl i32 %LocalID_Z, 8
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = add i32 %LocalID_X, [[TMP3]]
; CHECK:    [[TMP5:%.*]] = add i32 %LocalID_Y, %LocalID_Z
; CHECK:    [[TMP6:%.*]] = shl i32 %LocalID_Y, 3
; CHECK:    [[TMP7:%.*]] = add i32 [[TMP6]], %LocalID_X
; CHECK:    [[TMP8:%.*]] = lshr i32 [[TMP7]], 4

  %flat = call i32 @dx.op.flattenedThreadIdInGroup.i32(i32 96)
  %f1 = urem i32 %flat, 18
  %f2 = udiv i32 %flat, 18
  %f3 = lshr i32 %flat, 4
  %f4 = trunc i32 %flat to i16
  %f5 = lshr i16 %f4, 1

  %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
  %LocalID_Z = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 19)
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %1 = shl i32 %LocalID_Y, 4
  %2 = shl i32 %LocalID_Z, 8
  %3 = add i32 %1, %2
  %4 = add i32 %LocalID_X, %3

  %5 = add i32 %LocalID_Y, %LocalID_Z
  %6 = shl i32 %5, 8
  %7 = add i32 %LocalID_X, %6

  %8 = shl i32 %LocalID_Y, 3
  %9 = add i32 %8, %LocalID_X
  %10 = lshr i32 %9, 4

  ret void
}

declare i32 @dx.op.flattenedThreadIdInGroup.i32(i32) #0
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0
