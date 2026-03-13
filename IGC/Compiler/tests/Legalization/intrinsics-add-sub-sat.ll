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

; ------------------------------------------------
; Vector saturation intrinsics tests - <4 x i64>
; ------------------------------------------------

define <4 x i64> @test_usub_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_usub_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sub <4 x i64> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ugt <4 x i64> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <4 x i1> [[TMP2]], <4 x i64> zeroinitializer, <4 x i64> [[TMP1]]
; CHECK:    ret <4 x i64> [[TMP3]]
;
  %1 = call <4 x i64> @llvm.usub.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_uadd_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_uadd_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add <4 x i64> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ult <4 x i64> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <4 x i1> [[TMP2]], <4 x i64> <i64 -1, i64 -1, i64 -1, i64 -1>, <4 x i64> [[TMP1]]
; CHECK:    ret <4 x i64> [[TMP3]]
;
  %1 = call <4 x i64> @llvm.uadd.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_ssub_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_ssub_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i64> [[S1]] to <4 x i64>
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i64> [[S2]] to <4 x i64>
; CHECK:    [[TMP3:%.*]] = sub <4 x i64> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <4 x i64> [[TMP2]], <i64 -1, i64 -1, i64 -1, i64 -1>
; CHECK:    [[TMP5:%.*]] = xor <4 x i64> [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor <4 x i64> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor <4 x i64> [[TMP5]], <i64 -1, i64 -1, i64 -1, i64 -1>
; CHECK:    [[TMP8:%.*]] = and <4 x i64> [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = icmp slt <4 x i64> [[TMP8]], zeroinitializer
; CHECK:    [[TMP10:%.*]] = icmp slt <4 x i64> zeroinitializer, [[S2]]
; CHECK:    [[TMP11:%.*]] = select <4 x i1> [[TMP10]], <4 x i64> <i64 -9223372036854775808, i64 -9223372036854775808, i64 -9223372036854775808, i64 -9223372036854775808>, <4 x i64> <i64 9223372036854775807, i64 9223372036854775807, i64 9223372036854775807, i64 9223372036854775807>
; CHECK:    [[TMP12:%.*]] = select <4 x i1> [[TMP9]], <4 x i64> [[TMP11]], <4 x i64> [[TMP3]]
; CHECK:    ret <4 x i64> [[TMP12]]
;
  %1 = call <4 x i64> @llvm.ssub.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_sadd_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_sadd_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i64> [[S1]] to <4 x i64>
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i64> [[S2]] to <4 x i64>
; CHECK:    [[TMP3:%.*]] = add <4 x i64> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <4 x i64> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor <4 x i64> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor <4 x i64> [[TMP4]], <i64 -1, i64 -1, i64 -1, i64 -1>
; CHECK:    [[TMP7:%.*]] = and <4 x i64> [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt <4 x i64> [[TMP7]], zeroinitializer
; CHECK:    [[TMP9:%.*]] = icmp slt <4 x i64> zeroinitializer, [[S2]]
; CHECK:    [[TMP10:%.*]] = select <4 x i1> [[TMP9]], <4 x i64> <i64 9223372036854775807, i64 9223372036854775807, i64 9223372036854775807, i64 9223372036854775807>, <4 x i64> <i64 -9223372036854775808, i64 -9223372036854775808, i64 -9223372036854775808, i64 -9223372036854775808>
; CHECK:    [[TMP11:%.*]] = select <4 x i1> [[TMP8]], <4 x i64> [[TMP10]], <4 x i64> [[TMP3]]
; CHECK:    ret <4 x i64> [[TMP11]]
;
  %1 = call <4 x i64> @llvm.sadd.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

; ------------------------------------------------
; Vector saturation intrinsics tests - <4 x i32>
; ------------------------------------------------

define <4 x i32> @test_usub_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_usub_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sub <4 x i32> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ugt <4 x i32> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <4 x i1> [[TMP2]], <4 x i32> zeroinitializer, <4 x i32> [[TMP1]]
; CHECK:    ret <4 x i32> [[TMP3]]
;
  %1 = call <4 x i32> @llvm.usub.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_uadd_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_uadd_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add <4 x i32> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ult <4 x i32> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <4 x i1> [[TMP2]], <4 x i32> <i32 -1, i32 -1, i32 -1, i32 -1>, <4 x i32> [[TMP1]]
; CHECK:    ret <4 x i32> [[TMP3]]
;
  %1 = call <4 x i32> @llvm.uadd.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_ssub_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_ssub_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> [[S1]] to <4 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i32> [[S2]] to <4 x i32>
; CHECK:    [[TMP3:%.*]] = sub <4 x i32> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <4 x i32> [[TMP2]], <i32 -1, i32 -1, i32 -1, i32 -1>
; CHECK:    [[TMP5:%.*]] = xor <4 x i32> [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor <4 x i32> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor <4 x i32> [[TMP5]], <i32 -1, i32 -1, i32 -1, i32 -1>
; CHECK:    [[TMP8:%.*]] = and <4 x i32> [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = icmp slt <4 x i32> [[TMP8]], zeroinitializer
; CHECK:    [[TMP10:%.*]] = icmp slt <4 x i32> zeroinitializer, [[S2]]
; CHECK:    [[TMP11:%.*]] = select <4 x i1> [[TMP10]], <4 x i32> <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>, <4 x i32> <i32 2147483647, i32 2147483647, i32 2147483647, i32 2147483647>
; CHECK:    [[TMP12:%.*]] = select <4 x i1> [[TMP9]], <4 x i32> [[TMP11]], <4 x i32> [[TMP3]]
; CHECK:    ret <4 x i32> [[TMP12]]
;
  %1 = call <4 x i32> @llvm.ssub.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_sadd_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_sadd_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> [[S1]] to <4 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i32> [[S2]] to <4 x i32>
; CHECK:    [[TMP3:%.*]] = add <4 x i32> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <4 x i32> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor <4 x i32> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor <4 x i32> [[TMP4]], <i32 -1, i32 -1, i32 -1, i32 -1>
; CHECK:    [[TMP7:%.*]] = and <4 x i32> [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt <4 x i32> [[TMP7]], zeroinitializer
; CHECK:    [[TMP9:%.*]] = icmp slt <4 x i32> zeroinitializer, [[S2]]
; CHECK:    [[TMP10:%.*]] = select <4 x i1> [[TMP9]], <4 x i32> <i32 2147483647, i32 2147483647, i32 2147483647, i32 2147483647>, <4 x i32> <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>
; CHECK:    [[TMP11:%.*]] = select <4 x i1> [[TMP8]], <4 x i32> [[TMP10]], <4 x i32> [[TMP3]]
; CHECK:    ret <4 x i32> [[TMP11]]
;
  %1 = call <4 x i32> @llvm.sadd.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

; ------------------------------------------------
; Vector saturation intrinsics tests - <8 x i16>
; ------------------------------------------------

define <8 x i16> @test_usub_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_usub_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sub <8 x i16> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ugt <8 x i16> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <8 x i1> [[TMP2]], <8 x i16> zeroinitializer, <8 x i16> [[TMP1]]
; CHECK:    ret <8 x i16> [[TMP3]]
;
  %1 = call <8 x i16> @llvm.usub.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_uadd_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_uadd_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add <8 x i16> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ult <8 x i16> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <8 x i1> [[TMP2]], <8 x i16> <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>, <8 x i16> [[TMP1]]
; CHECK:    ret <8 x i16> [[TMP3]]
;
  %1 = call <8 x i16> @llvm.uadd.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_ssub_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_ssub_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <8 x i16> [[S1]] to <8 x i16>
; CHECK:    [[TMP2:%.*]] = bitcast <8 x i16> [[S2]] to <8 x i16>
; CHECK:    [[TMP3:%.*]] = sub <8 x i16> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <8 x i16> [[TMP2]], <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
; CHECK:    [[TMP5:%.*]] = xor <8 x i16> [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor <8 x i16> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor <8 x i16> [[TMP5]], <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
; CHECK:    [[TMP8:%.*]] = and <8 x i16> [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = icmp slt <8 x i16> [[TMP8]], zeroinitializer
; CHECK:    [[TMP10:%.*]] = icmp slt <8 x i16> zeroinitializer, [[S2]]
; CHECK:    [[TMP11:%.*]] = select <8 x i1> [[TMP10]], <8 x i16> <i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768>, <8 x i16> <i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767>
; CHECK:    [[TMP12:%.*]] = select <8 x i1> [[TMP9]], <8 x i16> [[TMP11]], <8 x i16> [[TMP3]]
; CHECK:    ret <8 x i16> [[TMP12]]
;
  %1 = call <8 x i16> @llvm.ssub.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_sadd_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_sadd_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <8 x i16> [[S1]] to <8 x i16>
; CHECK:    [[TMP2:%.*]] = bitcast <8 x i16> [[S2]] to <8 x i16>
; CHECK:    [[TMP3:%.*]] = add <8 x i16> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <8 x i16> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor <8 x i16> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor <8 x i16> [[TMP4]], <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
; CHECK:    [[TMP7:%.*]] = and <8 x i16> [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt <8 x i16> [[TMP7]], zeroinitializer
; CHECK:    [[TMP9:%.*]] = icmp slt <8 x i16> zeroinitializer, [[S2]]
; CHECK:    [[TMP10:%.*]] = select <8 x i1> [[TMP9]], <8 x i16> <i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767, i16 32767>, <8 x i16> <i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768, i16 -32768>
; CHECK:    [[TMP11:%.*]] = select <8 x i1> [[TMP8]], <8 x i16> [[TMP10]], <8 x i16> [[TMP3]]
; CHECK:    ret <8 x i16> [[TMP11]]
;
  %1 = call <8 x i16> @llvm.sadd.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

; ------------------------------------------------
; Vector saturation intrinsics tests - <16 x i8>
; ------------------------------------------------

define <16 x i8> @test_usub_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_usub_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = sub <16 x i8> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ugt <16 x i8> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <16 x i1> [[TMP2]], <16 x i8> zeroinitializer, <16 x i8> [[TMP1]]
; CHECK:    ret <16 x i8> [[TMP3]]
;
  %1 = call <16 x i8> @llvm.usub.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_uadd_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_uadd_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add <16 x i8> [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = icmp ult <16 x i8> [[TMP1]], [[S1]]
; CHECK:    [[TMP3:%.*]] = select <16 x i1> [[TMP2]], <16 x i8> <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>, <16 x i8> [[TMP1]]
; CHECK:    ret <16 x i8> [[TMP3]]
;
  %1 = call <16 x i8> @llvm.uadd.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_sadd_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_sadd_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <16 x i8> [[S1]] to <16 x i8>
; CHECK:    [[TMP2:%.*]] = bitcast <16 x i8> [[S2]] to <16 x i8>
; CHECK:    [[TMP3:%.*]] = add <16 x i8> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <16 x i8> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor <16 x i8> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor <16 x i8> [[TMP4]], <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>
; CHECK:    [[TMP7:%.*]] = and <16 x i8> [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt <16 x i8> [[TMP7]], zeroinitializer
; CHECK:    [[TMP9:%.*]] = icmp slt <16 x i8> zeroinitializer, [[S2]]
; CHECK:    [[TMP10:%.*]] = select <16 x i1> [[TMP9]], <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, <16 x i8> <i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128>
; CHECK:    [[TMP11:%.*]] = select <16 x i1> [[TMP8]], <16 x i8> [[TMP10]], <16 x i8> [[TMP3]]
; CHECK:    ret <16 x i8> [[TMP11]]
;
  %1 = call <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_ssub_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_ssub_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast <16 x i8> [[S1]] to <16 x i8>
; CHECK:    [[TMP2:%.*]] = bitcast <16 x i8> [[S2]] to <16 x i8>
; CHECK:    [[TMP3:%.*]] = sub <16 x i8> [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor <16 x i8> [[TMP2]], <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>
; CHECK:    [[TMP5:%.*]] = xor <16 x i8> [[TMP1]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor <16 x i8> [[TMP3]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = xor <16 x i8> [[TMP5]], <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>
; CHECK:    [[TMP8:%.*]] = and <16 x i8> [[TMP7]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = icmp slt <16 x i8> [[TMP8]], zeroinitializer
; CHECK:    [[TMP10:%.*]] = icmp slt <16 x i8> zeroinitializer, [[S2]]
; CHECK:    [[TMP11:%.*]] = select <16 x i1> [[TMP10]], <16 x i8> <i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128, i8 -128>, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>
; CHECK:    [[TMP12:%.*]] = select <16 x i1> [[TMP9]], <16 x i8> [[TMP11]], <16 x i8> [[TMP3]]
; CHECK:    ret <16 x i8> [[TMP12]]
;
  %1 = call <16 x i8> @llvm.ssub.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

; Scalar intrinsics
declare i64 @llvm.usub.sat.i64(i64, i64)
declare i64 @llvm.uadd.sat.i64(i64, i64)
declare i64 @llvm.ssub.sat.i64(i64, i64)
declare i64 @llvm.sadd.sat.i64(i64, i64)

;  Declarations - vector i64
declare <4 x i64> @llvm.usub.sat.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.uadd.sat.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.ssub.sat.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.sadd.sat.v4i64(<4 x i64>, <4 x i64>)

; Declarations - vector i32
declare <4 x i32> @llvm.usub.sat.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.uadd.sat.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.ssub.sat.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.sadd.sat.v4i32(<4 x i32>, <4 x i32>)

; Declarations - vector i16
declare <8 x i16> @llvm.usub.sat.v8i16(<8 x i16>, <8 x i16>)
declare <8 x i16> @llvm.uadd.sat.v8i16(<8 x i16>, <8 x i16>)
declare <8 x i16> @llvm.ssub.sat.v8i16(<8 x i16>, <8 x i16>)
declare <8 x i16> @llvm.sadd.sat.v8i16(<8 x i16>, <8 x i16>)

; Declarations - vector i8
declare <16 x i8> @llvm.usub.sat.v16i8(<16 x i8>, <16 x i8>)
declare <16 x i8> @llvm.uadd.sat.v16i8(<16 x i8>, <16 x i8>)
declare <16 x i8> @llvm.ssub.sat.v16i8(<16 x i8>, <16 x i8>)
declare <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8>, <16 x i8>)

!igc.functions = !{!0, !1, !2, !3,
                   !4, !5, !6, !7,
                   !8, !9, !10, !11,
                   !12, !13, !14, !15,
                   !16, !17, !18, !19}

!0  = !{i64 (i64, i64)* @test_usub, !20}
!1  = !{i64 (i64, i64)* @test_uadd, !20}
!2  = !{i64 (i64, i64)* @test_ssub, !20}
!3  = !{i64 (i64, i64)* @test_sadd, !20}
!4  = !{<4 x i64> (<4 x i64>, <4 x i64>)* @test_usub_vec, !20}
!5  = !{<4 x i64> (<4 x i64>, <4 x i64>)* @test_uadd_vec, !20}
!6  = !{<4 x i64> (<4 x i64>, <4 x i64>)* @test_ssub_vec, !20}
!7  = !{<4 x i64> (<4 x i64>, <4 x i64>)* @test_sadd_vec, !20}
!8  = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_usub_vec_i32, !20}
!9  = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_uadd_vec_i32, !20}
!10 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_ssub_vec_i32, !20}
!11 = !{<4 x i32> (<4 x i32>, <4 x i32>)* @test_sadd_vec_i32, !20}
!12 = !{<8 x i16> (<8 x i16>, <8 x i16>)* @test_usub_vec_i16, !20}
!13 = !{<8 x i16> (<8 x i16>, <8 x i16>)* @test_uadd_vec_i16, !20}
!14 = !{<8 x i16> (<8 x i16>, <8 x i16>)* @test_ssub_vec_i16, !20}
!15 = !{<8 x i16> (<8 x i16>, <8 x i16>)* @test_sadd_vec_i16, !20}
!16 = !{<16 x i8> (<16 x i8>, <16 x i8>)* @test_usub_vec_i8, !20}
!17 = !{<16 x i8> (<16 x i8>, <16 x i8>)* @test_uadd_vec_i8, !20}
!18 = !{<16 x i8> (<16 x i8>, <16 x i8>)* @test_ssub_vec_i8, !20}
!19 = !{<16 x i8> (<16 x i8>, <16 x i8>)* @test_sadd_vec_i8, !20}
!20 = !{}