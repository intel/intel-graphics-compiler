;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : Instructions
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PAS

define i8 @test_add(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_add(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = add i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = add i8 %src1, %src2
  ret i8 %1
}

define i8 @test_sub(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_sub(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = sub i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = sub i8 %src1, %src2
  ret i8 %1
}

define i8 @test_mul(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_mul(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = mul i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = mul i8 %src1, %src2
  ret i8 %1
}

define i8 @test_and(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_and(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = and i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = and i8 %src1, %src2
  ret i8 %1
}

define i8 @test_or(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_or(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = or i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = or i8 %src1, %src2
  ret i8 %1
}

define i8 @test_xor(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_xor(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = xor i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = xor i8 %src1, %src2
  ret i8 %1
}

define i8 @test_ashr(i8 %src1) {
; CHECK-LABEL: @test_ashr(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC1]] to i16
; CHECK:    [[B2S2:%.*]] = ashr i16 [[B2S1]], 2
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = ashr i8 %src1, 2
  ret i8 %1
}

define i8 @test_lshr(i8 %src1) {
; CHECK-LABEL: @test_lshr(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = zext i8 [[SRC1]] to i16
; CHECK:    [[B2S2:%.*]] = lshr i16 [[B2S1]], 3
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = lshr i8 %src1, 3
  ret i8 %1
}

define i8 @test_shl(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_shl(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = shl i16 [[B2S]], [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = shl i8 %src1, %src2
  ret i8 %1
}

define i8 @test_select(i1 %cond, i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_select(
; CHECK:    [[B2S:%.*]] = sext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = select i1 [[COND:%.*]], i16 [[B2S]], i16 [[B2S1]]
; CHECK:    [[TMP1:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP1]]
;
  %1 = select i1 %cond, i8 %src1, i8 %src2
  ret i8 %1
}


define i8 @test_extractelement(<2 x i8> %src1, i32 %src2) {
; CHECK-LABEL: @test_extractelement(
; CHECK:    [[TMP1:%.*]] = extractelement <2 x i8> [[SRC1:%.*]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i8> [[SRC1]], i64 1
; CHECK:    [[B2S:%.*]] = sext i8 [[TMP1]] to i16
; CHECK:    [[B2S1:%.*]] = sext i8 [[TMP2]] to i16
; CHECK:    [[TMP3:%.*]] = insertelement <2 x i16> undef, i16 [[B2S]], i64 0
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i16> [[TMP3]], i16 [[B2S1]], i64 1
; CHECK:    [[B2S2:%.*]] = extractelement <2 x i16> [[TMP4]], i32 [[SRC2:%.*]]
; CHECK:    [[TMP5:%.*]] = trunc i16 [[B2S2]] to i8
; CHECK:    ret i8 [[TMP5]]
;
  %1 = extractelement <2 x i8> %src1, i32 %src2
  ret i8 %1
}

define i1 @test_icmp(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_icmp(
; CHECK:    [[B2S:%.*]] = zext i8 [[SRC1:%.*]] to i16
; CHECK:    [[B2S1:%.*]] = zext i8 [[SRC2:%.*]] to i16
; CHECK:    [[B2S2:%.*]] = icmp ugt i16 [[B2S]], [[B2S1]]
; CHECK:    ret i1 [[B2S2]]
;
  %1 = icmp ugt i8 %src1, %src2
  ret i1 %1
}

define i8 @test_ptrtoint(i8* %src) {
; CHECK-LABEL: @test_ptrtoint(
; CHECK:    [[P2B:%.*]] = ptrtoint i8* %src to i8
; CHECK:    [[B2S:%.*]] = sext i8 [[P2B]] to i16
; CHECK:    [[AND:%.*]] = and i16 [[B2S]], 7
; CHECK:    [[TRUNC:%.*]] = trunc i16 [[AND]] to i8
; CHECK:    ret i8 [[TRUNC]]

  %1 = ptrtoint i8* %src to i8
  %2 = and i8 %1, 7
  ret i8 %2
}
