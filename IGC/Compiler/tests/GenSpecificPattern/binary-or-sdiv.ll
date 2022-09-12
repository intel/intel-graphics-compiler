;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-gen-specific-pattern -check-debugify -S  < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: Or patterns
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_and_or(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_and_or(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[SRC1:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[SRC2:%.*]] to <2 x i32>
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP3]], i32 1
; CHECK:    [[TMP5:%.*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i32> [[TMP5]], i32 [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = bitcast <2 x i32> [[TMP6]] to i64
; CHECK:    call void @use.i64(i64 [[TMP7]])
; CHECK:    ret void
;
  %1 = and i64 %src1, 4294967295
  %2 = shl i64 %src2, 32
  %3 = or i64 %1, %2
  call void @use.i64(i64 %3)
  ret void
}

define spir_kernel void @test_and_or2(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_and_or2(
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC2:%.*]] to i32
; CHECK:    [[TMP2:%.*]] = bitcast i64 [[SRC1:%.*]] to <2 x i32>
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP1]], i32 1
; CHECK:    [[TMP6:%.*]] = bitcast <2 x i32> [[TMP5]] to i64
; CHECK:    call void @use.i64(i64 [[TMP6]])
; CHECK:    ret void
;
  %1 = trunc i64 %src2 to i32
  %2 = and i64 %src1, 4294967295
  %3 = insertelement <2 x i32> <i32 0, i32 undef>, i32 %1, i32 1
  %4 = bitcast <2 x i32> %3 to i64
  %5 = or i64 %2, %4
  call void @use.i64(i64 %5)
  ret void
}

define spir_kernel void @test_shl_or(i64 %src1) {
; CHECK-LABEL: @test_shl_or(
; CHECK:    [[TMP1:%.*]] = shl i64 [[SRC1:%.*]], 3
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 7
; CHECK:    call void @use.i64(i64 [[TMP2]])
; CHECK:    ret void
;
  %1 = shl i64 %src1, 3
  %2 = or i64 %1, 7
  call void @use.i64(i64 %2)
  ret void
}

define spir_kernel void @test_sdiv64(i64 %src1) {
; CHECK-LABEL: @test_sdiv64(
; CHECK:    [[TMP1:%.*]] = lshr i64 [[SRC1:%.*]], 63
; CHECK:    [[TMP2:%.*]] = add i64 [[SRC1]], [[TMP1]]
; CHECK:    [[TMP3:%.*]] = ashr i64 [[TMP2]], 1
; CHECK:    call void @use.i64(i64 [[TMP3]])
; CHECK:    ret void
;
  %1 = sdiv i64 %src1, 2
  call void @use.i64(i64 %1)
  ret void
}

define spir_kernel void @test_sdiv32(i32 %src1) {
; CHECK-LABEL: @test_sdiv32(
; CHECK:    [[TMP1:%.*]] = lshr i32 [[SRC1:%.*]], 31
; CHECK:    [[TMP2:%.*]] = add i32 [[SRC1]], [[TMP1]]
; CHECK:    [[TMP3:%.*]] = ashr i32 [[TMP2]], 1
; CHECK:    [[TMP4:%.*]] = ashr i32 [[TMP3]], 31
; CHECK:    [[TMP5:%.*]] = lshr i32 [[TMP4]], 30
; CHECK:    [[TMP6:%.*]] = add i32 [[TMP3]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = ashr i32 [[TMP6]], 2
; CHECK:    call void @use.i32(i32 [[TMP7]])
; CHECK:    ret void
;
  %1 = sdiv i32 %src1, 2
  %2 = sdiv i32 %1, 4
  call void @use.i32(i32 %2)
  ret void
}

declare void @use.i64(i64)
declare void @use.i32(i32)
