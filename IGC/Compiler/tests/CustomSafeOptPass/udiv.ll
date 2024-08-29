;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s

; @llvm.genx.GenISA.simdSize() is assumed to be 8/16/32 thus udiv could be safely
; substituted by three shifts right

define i32 @test_customsafe(i32 %a) {
; CHECK-LABEL: @test_customsafe(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:    [[TMP2:%.*]] = lshr i32 [[TMP1]], 4
; CHECK:    [[TMP3:%.*]] = lshr i32 [[A:%.*]], 3
; CHECK:    [[TMP4:%.*]] = lshr i32 [[TMP3]], [[TMP2]]
; CHECK:    ret i32 [[TMP4]]
;
  %1 = call i32 @llvm.genx.GenISA.simdSize()
  %2 = udiv i32 %a, %1
  ret i32 %2
}

define i8 @test_customsafe_with_trunc(i8 %a) {
; CHECK-LABEL: @test_customsafe_with_trunc(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:    [[TMP2:%.*]] = trunc i32 [[TMP1]] to i8
; CHECK:    [[TMP3:%.*]] = lshr i8 [[TMP2]], 4
; CHECK:    [[TMP4:%.*]] = lshr i8 [[A:%.*]], 3
; CHECK:    [[TMP5:%.*]] = lshr i8 [[TMP4]], [[TMP3]]
; CHECK:    ret i8 [[TMP5]]
;
  %1 = call i32 @llvm.genx.GenISA.simdSize()
  %2 = trunc i32 %1 to i8
  %3 = udiv i8 %a, %2
  ret i8 %3
}

declare i32 @llvm.genx.GenISA.simdSize()
