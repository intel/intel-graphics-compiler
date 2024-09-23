;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformpvc -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Checks legalization of Arithmetic with Overflow Intrinsics - sadd/ssub.with.overflow
; to add/sub for platforms that dont support byte arith

define i8 @test_sadd(i8 %s1, i8 %s2) {
; CHECK-LABEL: define i8 @test_sadd(
; CHECK-SAME: i8 [[S1:%.*]], i8 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i8 [[S1]] to i8
; CHECK:    [[TMP2:%.*]] = bitcast i8 [[S2]] to i8
; CHECK:    [[TMP3:%.*]] = add i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i8 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i8 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = and i8 [[TMP7]], -128
; CHECK:    [[TMP9:%.*]] = icmp ne i8 [[TMP8]], 0
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i8 [[TMP3]], i8 -112
; CHECK:    ret i8 [[TMP10]]
;
  %1 = call {i8, i1} @llvm.sadd.with.overflow.i8(i8 %s1, i8 %s2)
  %2 = extractvalue {i8, i1} %1, 0
  %3 = extractvalue {i8, i1} %1, 1
  %4 = select i1 %3, i8 %2, i8 144
  ret i8 %4
}

define i8 @test_ssub(i8 %s1, i8 %s2) {
; CHECK-LABEL: define i8 @test_ssub(
; CHECK-SAME: i8 [[S1:%.*]], i8 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i8 [[S1]] to i8
; CHECK:    [[TMP2:%.*]] = bitcast i8 [[S2]] to i8
; CHECK:    [[TMP3:%.*]] = sub i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i8 [[TMP2]], -1
; CHECK:    [[TMP5:%.*]] = xor i8 [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor i8 [[TMP5]], -1
; CHECK:    [[TMP8:%.*]] = and i8 [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = and i8 [[TMP8]], -128
; CHECK:    [[TMP10:%.*]] = icmp ne i8 [[TMP9]], 0
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i8 [[TMP3]], i8 -112
; CHECK:    ret i8 [[TMP11]]
;
  %1 = call {i8, i1} @llvm.ssub.with.overflow.i8(i8 %s1, i8 %s2)
  %2 = extractvalue {i8, i1} %1, 0
  %3 = extractvalue {i8, i1} %1, 1
  %4 = select i1 %3, i8 %2, i8 144
  ret i8 %4
}

declare {i8, i1} @llvm.sadd.with.overflow.i8(i8 %a, i8 %b)
declare {i8, i1} @llvm.ssub.with.overflow.i8(i8 %a, i8 %b)

!igc.functions = !{!0, !1}

!0 = !{i8 (i8, i8)* @test_ssub, !2}
!1 = !{i8 (i8, i8)* @test_sadd, !2}
!2 = !{}
