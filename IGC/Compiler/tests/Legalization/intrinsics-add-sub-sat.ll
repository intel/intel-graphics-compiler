;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; TODO: remove nop bitcasts from signed sat intrinsics
;
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------
; Checks legalization of Saturation Arithmetic Intrinsics - (u|s)add/sub.sat
; to add/sub with saturation checks

define i64 @test_usub(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_usub(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sub i64 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ugt i64 [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select i1 [[TMP2]], i64 0, i64 [[TMP1]]
; CHECK:    ret i64 [[TMP3]]
;
  %1 = call i64 @llvm.usub.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

define i64 @test_uadd(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_uadd(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add i64 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ult i64 [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select i1 [[TMP2]], i64 -1, i64 [[TMP1]]
; CHECK:    ret i64 [[TMP3]]
;
  %1 = call i64 @llvm.uadd.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

define i64 @test_ssub(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_ssub(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[S1]] to i64
; CHECK:    [[TMP2:%.*]] = bitcast i64 [[S2]] to i64
; CHECK:    [[TMP3:%.*]] = sub i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i64 [[TMP2]], -1
; CHECK:    [[TMP5:%.*]] = xor i64 [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor i64 [[TMP5]], -1
; CHECK:    [[TMP8:%.*]] = and i64 [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = icmp slt i64 [[TMP8]], 0
; CHECK:    [[TMP10:%.*]] = icmp slt i64 0, [[S2]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP9]], i64 [[TMP11]], i64 [[TMP3]]
; CHECK:    ret i64 [[TMP12]]
;
  %1 = call i64 @llvm.ssub.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

define i64 @test_sadd(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_sadd(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[S1]] to i64
; CHECK:    [[TMP2:%.*]] = bitcast i64 [[S2]] to i64
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i64 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i64 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt i64 [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = icmp slt i64 0, [[S2]]
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i64 9223372036854775807, i64 -9223372036854775808
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP8]], i64 [[TMP10]], i64 [[TMP3]]
; CHECK:    ret i64 [[TMP11]]
;
  %1 = call i64 @llvm.sadd.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

declare i64 @llvm.usub.sat.i64(i64, i64)
declare i64 @llvm.uadd.sat.i64(i64, i64)
declare i64 @llvm.ssub.sat.i64(i64, i64)
declare i64 @llvm.sadd.sat.i64(i64, i64)

!igc.functions = !{!0, !1, !2, !3}

!0 = !{i64 (i64, i64)* @test_usub, !4}
!1 = !{i64 (i64, i64)* @test_uadd, !4}
!2 = !{i64 (i64, i64)* @test_ssub, !4}
!3 = !{i64 (i64, i64)* @test_sadd, !4}
!4 = !{}
