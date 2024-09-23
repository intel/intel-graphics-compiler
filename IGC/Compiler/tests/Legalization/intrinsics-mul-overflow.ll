;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Checks legalization of Arithmetic with Overflow Intrinsics - (u|s)mul.with.overflow
; to mulh

define i64 @test_umul(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_umul(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i64 [[S1]], [[S2]]
; CHECK:    [[U_LO32:%.*]] = and i64 [[S1]], 4294967295
; CHECK:    [[U_HI32:%.*]] = lshr i64 [[S1]], 32
; CHECK:    [[V_LO32:%.*]] = and i64 [[S2]], 4294967295
; CHECK:    [[V_HI32:%.*]] = lshr i64 [[S2]], 32
; CHECK:    [[W0:%.*]] = mul i64 [[U_LO32]], [[V_LO32]]
; CHECK:    [[TMP2:%.*]] = mul i64 [[U_HI32]], [[V_LO32]]
; CHECK:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK:    [[T:%.*]] = add i64 [[TMP2]], [[W0_LO32]]
; CHECK:    [[TMP3:%.*]] = mul i64 [[U_LO32]], [[V_HI32]]
; CHECK:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK:    [[W1:%.*]] = add i64 [[TMP3]], [[T_LO32]]
; CHECK:    [[TMP4:%.*]] = mul i64 [[U_HI32]], [[V_HI32]]
; CHECK:    [[T_HI32:%.*]] = lshr i64 [[T]], 32
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], [[T_HI32]]
; CHECK:    [[W1_LO32:%.*]] = lshr i64 [[W1]], 32
; CHECK:    [[UV:%.*]] = add i64 [[TMP5]], [[W1_LO32]]
; CHECK:    [[TMP6:%.*]] = icmp ne i64 [[UV]], 0
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP6]], i64 [[TMP1]], i64 144
; CHECK:    ret i64 [[TMP7]]
;
  %1 = call {i64, i1} @llvm.umul.with.overflow.i64(i64 %s1, i64 %s2)
  %2 = extractvalue {i64, i1} %1, 0
  %3 = extractvalue {i64, i1} %1, 1
  %4 = select i1 %3, i64 %2, i64 144
  ret i64 %4
}

define i64 @test_smul(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_smul(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i64 [[S1]], [[S2]]
; CHECK:    [[U_LO32:%.*]] = and i64 [[S1]], 4294967295
; CHECK:    [[U_HI32:%.*]] = ashr i64 [[S1]], 32
; CHECK:    [[V_LO32:%.*]] = and i64 [[S2]], 4294967295
; CHECK:    [[V_HI32:%.*]] = ashr i64 [[S2]], 32
; CHECK:    [[W0:%.*]] = mul i64 [[U_LO32]], [[V_LO32]]
; CHECK:    [[TMP2:%.*]] = mul i64 [[U_HI32]], [[V_LO32]]
; CHECK:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK:    [[T:%.*]] = add i64 [[TMP2]], [[W0_LO32]]
; CHECK:    [[TMP3:%.*]] = mul i64 [[U_LO32]], [[V_HI32]]
; CHECK:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK:    [[W1:%.*]] = add i64 [[TMP3]], [[T_LO32]]
; CHECK:    [[TMP4:%.*]] = mul i64 [[U_HI32]], [[V_HI32]]
; CHECK:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], [[T_HI32]]
; CHECK:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK:    [[UV:%.*]] = add i64 [[TMP5]], [[W1_LO32]]
; CHECK:    [[TMP6:%.*]] = add i64 [[UV]], 1
; CHECK:    [[TMP7:%.*]] = icmp ugt i64 [[TMP6]], 1
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP7]], i64 [[TMP1]], i64 144
; CHECK:    ret i64 [[TMP8]]
;
  %1 = call {i64, i1} @llvm.smul.with.overflow.i64(i64 %s1, i64 %s2)
  %2 = extractvalue {i64, i1} %1, 0
  %3 = extractvalue {i64, i1} %1, 1
  %4 = select i1 %3, i64 %2, i64 144
  ret i64 %4
}

define i8 @test_umul_i8(i8 %s1, i8 %s2) {
; CHECK-LABEL: define i8 @test_umul_i8(
; CHECK-SAME: i8 [[S1:%.*]], i8 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i8 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = zext i8 [[S1]] to i32
; CHECK:    [[TMP3:%.*]] = zext i8 [[S2]] to i32
; CHECK:    [[TMP4:%.*]] = mul i32 [[TMP2]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = icmp uge i32 [[TMP4]], 256
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP5]], i8 [[TMP1]], i8 -112
; CHECK:    ret i8 [[TMP6]]
;
  %1 = call {i8, i1} @llvm.umul.with.overflow.i8(i8 %s1, i8 %s2)
  %2 = extractvalue {i8, i1} %1, 0
  %3 = extractvalue {i8, i1} %1, 1
  %4 = select i1 %3, i8 %2, i8 144
  ret i8 %4
}

define i8 @test_smul_i8(i8 %s1, i8 %s2) {
; CHECK-LABEL: define i8 @test_smul_i8(
; CHECK-SAME: i8 [[S1:%.*]], i8 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = mul i8 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = sext i8 [[S1]] to i32
; CHECK:    [[TMP3:%.*]] = sext i8 [[S2]] to i32
; CHECK:    [[TMP4:%.*]] = mul i32 [[TMP2]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP4]], 128
; CHECK:    [[TMP6:%.*]] = icmp uge i32 [[TMP5]], 256
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP6]], i8 [[TMP1]], i8 -112
; CHECK:    ret i8 [[TMP7]]
;
  %1 = call {i8, i1} @llvm.smul.with.overflow.i8(i8 %s1, i8 %s2)
  %2 = extractvalue {i8, i1} %1, 0
  %3 = extractvalue {i8, i1} %1, 1
  %4 = select i1 %3, i8 %2, i8 144
  ret i8 %4
}


declare {i64, i1} @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
declare {i64, i1} @llvm.umul.with.overflow.i64(i64 %a, i64 %b)
declare {i8, i1} @llvm.smul.with.overflow.i8(i8 %a, i8 %b)
declare {i8, i1} @llvm.umul.with.overflow.i8(i8 %a, i8 %b)

!igc.functions = !{!0, !1, !2, !3}

!0 = !{i64 (i64, i64)* @test_umul, !4}
!1 = !{i64 (i64, i64)* @test_smul, !4}
!2 = !{i8 (i8, i8)* @test_umul_i8, !4}
!3 = !{i8 (i8, i8)* @test_smul_i8, !4}
!4 = !{}
