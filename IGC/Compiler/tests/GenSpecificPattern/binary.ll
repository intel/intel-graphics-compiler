;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern:
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_add(i64 %src1) {
; CHECK-LABEL: @test_add(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = add i64 %src1, 57
; CHECK:    [[TMP2:%[A-z0-9.]*]] = bitcast i64 [[TMP1]] to <2 x i32>
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK:    [[TMP4:%[A-z0-9.]*]] = insertelement <2 x i32> <i32 0, i32 undef>, i32 [[TMP3]], i32 1
; CHECK:    [[TMP5:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP4]] to i64
; CHECK:    call void @use.i64(i64 [[TMP5]])
; CHECK:    ret void
;
  %1 = add i64 %src1, 42
  %2 = add i64 %1, 15
  %3 = shl i64 %2, 32
  call void @use.i64(i64 %3)
  ret void
}

define void @test_trunc_hi(i64 %src1) {
; CHECK-LABEL: @test_trunc_hi(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = trunc i64 %src1 to i32
; CHECK:    [[TMP2:%[A-z0-9.]*]] = bitcast i32 [[TMP1]] to <2 x i16>
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i16> [[TMP2]], i32 0
; CHECK:    [[TMP4:%[A-z0-9.]*]] = sext i16 [[TMP3]] to i32
; CHECK:    call void @use.i32(i32 [[TMP4]])
; CHECK:    ret void
;
  %1 = trunc i64 %src1 to i32
  %2 = shl i32 %1, 16
  %3 = ashr i32 %2, 16
  call void @use.i32(i32 %3)
  ret void
}

define void @test_trunc_low(i64 %src1) {
; CHECK-LABEL: @test_trunc_low(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = trunc i64 %src1 to i32
; CHECK:    [[TMP2:%[A-z0-9.]*]] = bitcast i32 [[TMP1]] to <2 x i16>
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i16> [[TMP2]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = sext i16 [[TMP3]] to i32
; CHECK:    call void @use.i32(i32 [[TMP4]])
; CHECK:    ret void
;
  %1 = trunc i64 %src1 to i32
  %2 = ashr i32 %1, 16
  call void @use.i32(i32 %2)
  ret void
}

define void @test_and32(double %src1) {
; CHECK-LABEL: @test_and32(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast double %src1 to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP3:%[A-z0-9.]*]] = and i32 [[TMP2]], 2147483647
; CHECK:    call void @use.i32(i32 [[TMP3]])
; CHECK:    ret void
;
  %1 = fsub double -0.000000e+00, %src1
  %2 = bitcast double %1 to <2 x i32>
  %3 = extractelement <2 x i32> %2, i32 1
  %4 = and i32 %3, 2147483647
  call void @use.i32(i32 %4)
  ret void
}

define void @test_and64(double %src1) {
; CHECK-LABEL: @test_and64(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = fsub double -0.000000e+00, [[SRC1:%[A-z0-9.]*]]
; CHECK:    [[TMP2:%[A-z0-9.]*]] = bitcast double [[TMP1]] to i64
; CHECK:    [[TMP3:%[A-z0-9.]*]] = bitcast double [[SRC1]] to i64
; CHECK:    [[TMP4:%[A-z0-9.]*]] = and i64 [[TMP3]], 9223372032559808512
; CHECK:    call void @use.i64(i64 [[TMP4]])
; CHECK:    [[TMP5:%[A-z0-9.]*]] = bitcast i64 [[TMP2]] to <2 x i32>
; CHECK:    [[TMP6:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP5]], i32 1
; CHECK:    [[TMP7:%[A-z0-9.]*]] = insertelement <2 x i32> <i32 0, i32 undef>, i32 [[TMP6]], i32 1
; CHECK:    [[TMP8:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP7]] to i64
; CHECK:    call void @use.i64(i64 [[TMP8]])
; CHECK:    ret void
;
  %1 = fsub double -0.000000e+00, %src1
  %2 = bitcast double %1 to i64
  %3 = and i64 %2, 9223372032559808512
  call void @use.i64(i64 %3)
  %4 = and i64 %2, -4294967296
  call void @use.i64(i64 %4)
  ret void
}

declare void @use.i64(i64)
declare void @use.i32(i32)
