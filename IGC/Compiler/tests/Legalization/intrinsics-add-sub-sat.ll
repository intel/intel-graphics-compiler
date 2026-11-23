;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
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
; CHECK:    [[TMP1:%.*]] = sub i64 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = xor i64 [[S1]], [[S2]]
; CHECK:    [[TMP3:%.*]] = xor i64 [[TMP1]], [[S1]]
; CHECK:    [[TMP4:%.*]] = and i64 [[TMP2]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = icmp slt i64 [[TMP4]], 0
; CHECK:    [[TMP6:%.*]] = icmp slt i64 [[S1]], 0
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP6]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP5]], i64 [[TMP7]], i64 [[TMP1]]
; CHECK:    ret i64 [[TMP8]]
;
  %1 = call i64 @llvm.ssub.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

define i64 @test_sadd(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_sadd(
; CHECK-SAME: i64 [[S1:%.*]], i64 [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add i64 [[S1]], [[S2]]
; CHECK:    [[TMP2:%.*]] = xor i64 [[S1]], [[S2]]
; CHECK:    [[TMP3:%.*]] = xor i64 [[TMP1]], [[S1]]
; CHECK:    [[TMP4:%.*]] = xor i64 [[TMP2]], -1
; CHECK:    [[TMP5:%.*]] = and i64 [[TMP4]], [[TMP3]]
; CHECK:    [[TMP6:%.*]] = icmp slt i64 [[TMP5]], 0
; CHECK:    [[TMP7:%.*]] = icmp slt i64 [[S1]], 0
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP7]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP6]], i64 [[TMP8]], i64 [[TMP1]]
; CHECK:    ret i64 [[TMP9]]
;
  %1 = call i64 @llvm.sadd.sat.i64(i64 %s1, i64 %s2)
  ret i64 %1
}

; ------------------------------------------------
; Vector saturation intrinsics tests - <4 x i64>
; ------------------------------------------------
; Vector intrinsics are split into scalar ones, each lowered as above.

define <4 x i64> @test_usub_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_usub_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i64> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ugt i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i64 0, i64 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i64> poison, i64 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = sub i64 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ugt i64 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i64 0, i64 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i64> [[TMP6]], i64 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = sub i64 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ugt i64 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i64 0, i64 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <4 x i64> [[TMP12]], i64 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = sub i64 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ugt i64 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i64 0, i64 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i64> [[TMP18]], i64 [[TMP23]], i64 3
; CHECK:    ret <4 x i64> [[TMP24]]
;
  %1 = call <4 x i64> @llvm.usub.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_uadd_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_uadd_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i64> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ult i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i64 -1, i64 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i64> poison, i64 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = add i64 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ult i64 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i64 -1, i64 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i64> [[TMP6]], i64 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = add i64 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ult i64 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i64 -1, i64 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <4 x i64> [[TMP12]], i64 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = add i64 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ult i64 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i64 -1, i64 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i64> [[TMP18]], i64 [[TMP23]], i64 3
; CHECK:    ret <4 x i64> [[TMP24]]
;
  %1 = call <4 x i64> @llvm.uadd.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_ssub_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_ssub_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i64> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = and i64 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = icmp slt i64 [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = icmp slt i64 [[TMP1]], 0
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP8]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP7]], i64 [[TMP9]], i64 [[TMP3]]
; CHECK:    [[TMP11:%.*]] = insertelement <4 x i64> poison, i64 [[TMP10]], i64 0
; CHECK:    [[TMP12:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP14:%.*]] = sub i64 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP15:%.*]] = xor i64 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP16:%.*]] = xor i64 [[TMP14]], [[TMP12]]
; CHECK:    [[TMP17:%.*]] = and i64 [[TMP15]], [[TMP16]]
; CHECK:    [[TMP18:%.*]] = icmp slt i64 [[TMP17]], 0
; CHECK:    [[TMP19:%.*]] = icmp slt i64 [[TMP12]], 0
; CHECK:    [[TMP20:%.*]] = select i1 [[TMP19]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP21:%.*]] = select i1 [[TMP18]], i64 [[TMP20]], i64 [[TMP14]]
; CHECK:    [[TMP22:%.*]] = insertelement <4 x i64> [[TMP11]], i64 [[TMP21]], i64 1
; CHECK:    [[TMP23:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP24:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP25:%.*]] = sub i64 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP26:%.*]] = xor i64 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP27:%.*]] = xor i64 [[TMP25]], [[TMP23]]
; CHECK:    [[TMP28:%.*]] = and i64 [[TMP26]], [[TMP27]]
; CHECK:    [[TMP29:%.*]] = icmp slt i64 [[TMP28]], 0
; CHECK:    [[TMP30:%.*]] = icmp slt i64 [[TMP23]], 0
; CHECK:    [[TMP31:%.*]] = select i1 [[TMP30]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP32:%.*]] = select i1 [[TMP29]], i64 [[TMP31]], i64 [[TMP25]]
; CHECK:    [[TMP33:%.*]] = insertelement <4 x i64> [[TMP22]], i64 [[TMP32]], i64 2
; CHECK:    [[TMP34:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP35:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP36:%.*]] = sub i64 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP37:%.*]] = xor i64 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = xor i64 [[TMP36]], [[TMP34]]
; CHECK:    [[TMP39:%.*]] = and i64 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp slt i64 [[TMP39]], 0
; CHECK:    [[TMP41:%.*]] = icmp slt i64 [[TMP34]], 0
; CHECK:    [[TMP42:%.*]] = select i1 [[TMP41]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP40]], i64 [[TMP42]], i64 [[TMP36]]
; CHECK:    [[TMP44:%.*]] = insertelement <4 x i64> [[TMP33]], i64 [[TMP43]], i64 3
; CHECK:    ret <4 x i64> [[TMP44]]
;
  %1 = call <4 x i64> @llvm.ssub.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_sadd_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_sadd_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i64> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i64 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i64 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i64 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i64 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt i64 [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = icmp slt i64 [[TMP1]], 0
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP8]], i64 [[TMP10]], i64 [[TMP3]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i64> poison, i64 [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP15:%.*]] = add i64 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = xor i64 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP17:%.*]] = xor i64 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP18:%.*]] = xor i64 [[TMP16]], -1
; CHECK:    [[TMP19:%.*]] = and i64 [[TMP18]], [[TMP17]]
; CHECK:    [[TMP20:%.*]] = icmp slt i64 [[TMP19]], 0
; CHECK:    [[TMP21:%.*]] = icmp slt i64 [[TMP13]], 0
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP21]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP20]], i64 [[TMP22]], i64 [[TMP15]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i64> [[TMP12]], i64 [[TMP23]], i64 1
; CHECK:    [[TMP25:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP26:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP27:%.*]] = add i64 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = xor i64 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP29:%.*]] = xor i64 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP30:%.*]] = xor i64 [[TMP28]], -1
; CHECK:    [[TMP31:%.*]] = and i64 [[TMP30]], [[TMP29]]
; CHECK:    [[TMP32:%.*]] = icmp slt i64 [[TMP31]], 0
; CHECK:    [[TMP33:%.*]] = icmp slt i64 [[TMP25]], 0
; CHECK:    [[TMP34:%.*]] = select i1 [[TMP33]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP32]], i64 [[TMP34]], i64 [[TMP27]]
; CHECK:    [[TMP36:%.*]] = insertelement <4 x i64> [[TMP24]], i64 [[TMP35]], i64 2
; CHECK:    [[TMP37:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP38:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP39:%.*]] = add i64 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = xor i64 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = xor i64 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP42:%.*]] = xor i64 [[TMP40]], -1
; CHECK:    [[TMP43:%.*]] = and i64 [[TMP42]], [[TMP41]]
; CHECK:    [[TMP44:%.*]] = icmp slt i64 [[TMP43]], 0
; CHECK:    [[TMP45:%.*]] = icmp slt i64 [[TMP37]], 0
; CHECK:    [[TMP46:%.*]] = select i1 [[TMP45]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP44]], i64 [[TMP46]], i64 [[TMP39]]
; CHECK:    [[TMP48:%.*]] = insertelement <4 x i64> [[TMP36]], i64 [[TMP47]], i64 3
; CHECK:    ret <4 x i64> [[TMP48]]
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
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ugt i32 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i32 0, i32 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i32> poison, i32 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = sub i32 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ugt i32 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i32 0, i32 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i32> [[TMP6]], i32 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = sub i32 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ugt i32 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i32 0, i32 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <4 x i32> [[TMP12]], i32 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = sub i32 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ugt i32 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i32 0, i32 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i32> [[TMP18]], i32 [[TMP23]], i64 3
; CHECK:    ret <4 x i32> [[TMP24]]
;
  %1 = call <4 x i32> @llvm.usub.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_uadd_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_uadd_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ult i32 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i32 -1, i32 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i32> poison, i32 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = add i32 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ult i32 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i32 -1, i32 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i32> [[TMP6]], i32 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = add i32 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ult i32 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i32 -1, i32 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <4 x i32> [[TMP12]], i32 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = add i32 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ult i32 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i32 -1, i32 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i32> [[TMP18]], i32 [[TMP23]], i64 3
; CHECK:    ret <4 x i32> [[TMP24]]
;
  %1 = call <4 x i32> @llvm.uadd.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_ssub_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_ssub_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i32 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = and i32 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = icmp slt i32 [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = icmp slt i32 [[TMP1]], 0
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP8]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP7]], i32 [[TMP9]], i32 [[TMP3]]
; CHECK:    [[TMP11:%.*]] = insertelement <4 x i32> poison, i32 [[TMP10]], i64 0
; CHECK:    [[TMP12:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP14:%.*]] = sub i32 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP15:%.*]] = xor i32 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP16:%.*]] = xor i32 [[TMP14]], [[TMP12]]
; CHECK:    [[TMP17:%.*]] = and i32 [[TMP15]], [[TMP16]]
; CHECK:    [[TMP18:%.*]] = icmp slt i32 [[TMP17]], 0
; CHECK:    [[TMP19:%.*]] = icmp slt i32 [[TMP12]], 0
; CHECK:    [[TMP20:%.*]] = select i1 [[TMP19]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP21:%.*]] = select i1 [[TMP18]], i32 [[TMP20]], i32 [[TMP14]]
; CHECK:    [[TMP22:%.*]] = insertelement <4 x i32> [[TMP11]], i32 [[TMP21]], i64 1
; CHECK:    [[TMP23:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP24:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP25:%.*]] = sub i32 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP26:%.*]] = xor i32 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP27:%.*]] = xor i32 [[TMP25]], [[TMP23]]
; CHECK:    [[TMP28:%.*]] = and i32 [[TMP26]], [[TMP27]]
; CHECK:    [[TMP29:%.*]] = icmp slt i32 [[TMP28]], 0
; CHECK:    [[TMP30:%.*]] = icmp slt i32 [[TMP23]], 0
; CHECK:    [[TMP31:%.*]] = select i1 [[TMP30]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP32:%.*]] = select i1 [[TMP29]], i32 [[TMP31]], i32 [[TMP25]]
; CHECK:    [[TMP33:%.*]] = insertelement <4 x i32> [[TMP22]], i32 [[TMP32]], i64 2
; CHECK:    [[TMP34:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP35:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP36:%.*]] = sub i32 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP37:%.*]] = xor i32 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = xor i32 [[TMP36]], [[TMP34]]
; CHECK:    [[TMP39:%.*]] = and i32 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp slt i32 [[TMP39]], 0
; CHECK:    [[TMP41:%.*]] = icmp slt i32 [[TMP34]], 0
; CHECK:    [[TMP42:%.*]] = select i1 [[TMP41]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP40]], i32 [[TMP42]], i32 [[TMP36]]
; CHECK:    [[TMP44:%.*]] = insertelement <4 x i32> [[TMP33]], i32 [[TMP43]], i64 3
; CHECK:    ret <4 x i32> [[TMP44]]
;
  %1 = call <4 x i32> @llvm.ssub.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_sadd_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_sadd_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i32 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i32 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i32 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt i32 [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = icmp slt i32 [[TMP1]], 0
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP8]], i32 [[TMP10]], i32 [[TMP3]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i32> poison, i32 [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP15:%.*]] = add i32 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = xor i32 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP17:%.*]] = xor i32 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP18:%.*]] = xor i32 [[TMP16]], -1
; CHECK:    [[TMP19:%.*]] = and i32 [[TMP18]], [[TMP17]]
; CHECK:    [[TMP20:%.*]] = icmp slt i32 [[TMP19]], 0
; CHECK:    [[TMP21:%.*]] = icmp slt i32 [[TMP13]], 0
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP21]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP20]], i32 [[TMP22]], i32 [[TMP15]]
; CHECK:    [[TMP24:%.*]] = insertelement <4 x i32> [[TMP12]], i32 [[TMP23]], i64 1
; CHECK:    [[TMP25:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP26:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP27:%.*]] = add i32 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = xor i32 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP29:%.*]] = xor i32 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP30:%.*]] = xor i32 [[TMP28]], -1
; CHECK:    [[TMP31:%.*]] = and i32 [[TMP30]], [[TMP29]]
; CHECK:    [[TMP32:%.*]] = icmp slt i32 [[TMP31]], 0
; CHECK:    [[TMP33:%.*]] = icmp slt i32 [[TMP25]], 0
; CHECK:    [[TMP34:%.*]] = select i1 [[TMP33]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP32]], i32 [[TMP34]], i32 [[TMP27]]
; CHECK:    [[TMP36:%.*]] = insertelement <4 x i32> [[TMP24]], i32 [[TMP35]], i64 2
; CHECK:    [[TMP37:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP38:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP39:%.*]] = add i32 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = xor i32 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = xor i32 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP42:%.*]] = xor i32 [[TMP40]], -1
; CHECK:    [[TMP43:%.*]] = and i32 [[TMP42]], [[TMP41]]
; CHECK:    [[TMP44:%.*]] = icmp slt i32 [[TMP43]], 0
; CHECK:    [[TMP45:%.*]] = icmp slt i32 [[TMP37]], 0
; CHECK:    [[TMP46:%.*]] = select i1 [[TMP45]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP44]], i32 [[TMP46]], i32 [[TMP39]]
; CHECK:    [[TMP48:%.*]] = insertelement <4 x i32> [[TMP36]], i32 [[TMP47]], i64 3
; CHECK:    ret <4 x i32> [[TMP48]]
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
; CHECK:    [[TMP1:%.*]] = extractelement <8 x i16> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i16> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ugt i16 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i16 0, i16 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i16> poison, i16 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = sub i16 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ugt i16 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i16 0, i16 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <8 x i16> [[TMP6]], i16 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = sub i16 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ugt i16 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i16 0, i16 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <8 x i16> [[TMP12]], i16 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = sub i16 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ugt i16 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i16 0, i16 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <8 x i16> [[TMP18]], i16 [[TMP23]], i64 3
; CHECK:    [[TMP25:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP26:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP27:%.*]] = sub i16 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = icmp ugt i16 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP28]], i16 0, i16 [[TMP27]]
; CHECK:    [[TMP30:%.*]] = insertelement <8 x i16> [[TMP24]], i16 [[TMP29]], i64 4
; CHECK:    [[TMP31:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP32:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP33:%.*]] = sub i16 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = icmp ugt i16 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP34]], i16 0, i16 [[TMP33]]
; CHECK:    [[TMP36:%.*]] = insertelement <8 x i16> [[TMP30]], i16 [[TMP35]], i64 5
; CHECK:    [[TMP37:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP38:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP39:%.*]] = sub i16 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp ugt i16 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP40]], i16 0, i16 [[TMP39]]
; CHECK:    [[TMP42:%.*]] = insertelement <8 x i16> [[TMP36]], i16 [[TMP41]], i64 6
; CHECK:    [[TMP43:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP44:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP45:%.*]] = sub i16 [[TMP43]], [[TMP44]]
; CHECK:    [[TMP46:%.*]] = icmp ugt i16 [[TMP45]], [[TMP43]]
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP46]], i16 0, i16 [[TMP45]]
; CHECK:    [[TMP48:%.*]] = insertelement <8 x i16> [[TMP42]], i16 [[TMP47]], i64 7
; CHECK:    ret <8 x i16> [[TMP48]]
;
  %1 = call <8 x i16> @llvm.usub.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_uadd_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_uadd_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <8 x i16> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i16> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ult i16 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i16 -1, i16 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i16> poison, i16 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = add i16 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ult i16 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i16 -1, i16 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <8 x i16> [[TMP6]], i16 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = add i16 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ult i16 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i16 -1, i16 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <8 x i16> [[TMP12]], i16 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = add i16 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ult i16 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i16 -1, i16 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <8 x i16> [[TMP18]], i16 [[TMP23]], i64 3
; CHECK:    [[TMP25:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP26:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP27:%.*]] = add i16 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = icmp ult i16 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP28]], i16 -1, i16 [[TMP27]]
; CHECK:    [[TMP30:%.*]] = insertelement <8 x i16> [[TMP24]], i16 [[TMP29]], i64 4
; CHECK:    [[TMP31:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP32:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP33:%.*]] = add i16 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = icmp ult i16 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP34]], i16 -1, i16 [[TMP33]]
; CHECK:    [[TMP36:%.*]] = insertelement <8 x i16> [[TMP30]], i16 [[TMP35]], i64 5
; CHECK:    [[TMP37:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP38:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP39:%.*]] = add i16 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp ult i16 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP40]], i16 -1, i16 [[TMP39]]
; CHECK:    [[TMP42:%.*]] = insertelement <8 x i16> [[TMP36]], i16 [[TMP41]], i64 6
; CHECK:    [[TMP43:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP44:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP45:%.*]] = add i16 [[TMP43]], [[TMP44]]
; CHECK:    [[TMP46:%.*]] = icmp ult i16 [[TMP45]], [[TMP43]]
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP46]], i16 -1, i16 [[TMP45]]
; CHECK:    [[TMP48:%.*]] = insertelement <8 x i16> [[TMP42]], i16 [[TMP47]], i64 7
; CHECK:    ret <8 x i16> [[TMP48]]
;
  %1 = call <8 x i16> @llvm.uadd.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_ssub_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_ssub_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <8 x i16> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i16> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i16 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = and i16 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = icmp slt i16 [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = icmp slt i16 [[TMP1]], 0
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP8]], i16 -32768, i16 32767
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP7]], i16 [[TMP9]], i16 [[TMP3]]
; CHECK:    [[TMP11:%.*]] = insertelement <8 x i16> poison, i16 [[TMP10]], i64 0
; CHECK:    [[TMP12:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP14:%.*]] = sub i16 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP15:%.*]] = xor i16 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP16:%.*]] = xor i16 [[TMP14]], [[TMP12]]
; CHECK:    [[TMP17:%.*]] = and i16 [[TMP15]], [[TMP16]]
; CHECK:    [[TMP18:%.*]] = icmp slt i16 [[TMP17]], 0
; CHECK:    [[TMP19:%.*]] = icmp slt i16 [[TMP12]], 0
; CHECK:    [[TMP20:%.*]] = select i1 [[TMP19]], i16 -32768, i16 32767
; CHECK:    [[TMP21:%.*]] = select i1 [[TMP18]], i16 [[TMP20]], i16 [[TMP14]]
; CHECK:    [[TMP22:%.*]] = insertelement <8 x i16> [[TMP11]], i16 [[TMP21]], i64 1
; CHECK:    [[TMP23:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP24:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP25:%.*]] = sub i16 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP26:%.*]] = xor i16 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP27:%.*]] = xor i16 [[TMP25]], [[TMP23]]
; CHECK:    [[TMP28:%.*]] = and i16 [[TMP26]], [[TMP27]]
; CHECK:    [[TMP29:%.*]] = icmp slt i16 [[TMP28]], 0
; CHECK:    [[TMP30:%.*]] = icmp slt i16 [[TMP23]], 0
; CHECK:    [[TMP31:%.*]] = select i1 [[TMP30]], i16 -32768, i16 32767
; CHECK:    [[TMP32:%.*]] = select i1 [[TMP29]], i16 [[TMP31]], i16 [[TMP25]]
; CHECK:    [[TMP33:%.*]] = insertelement <8 x i16> [[TMP22]], i16 [[TMP32]], i64 2
; CHECK:    [[TMP34:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP35:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP36:%.*]] = sub i16 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP37:%.*]] = xor i16 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = xor i16 [[TMP36]], [[TMP34]]
; CHECK:    [[TMP39:%.*]] = and i16 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp slt i16 [[TMP39]], 0
; CHECK:    [[TMP41:%.*]] = icmp slt i16 [[TMP34]], 0
; CHECK:    [[TMP42:%.*]] = select i1 [[TMP41]], i16 -32768, i16 32767
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP40]], i16 [[TMP42]], i16 [[TMP36]]
; CHECK:    [[TMP44:%.*]] = insertelement <8 x i16> [[TMP33]], i16 [[TMP43]], i64 3
; CHECK:    [[TMP45:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP46:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP47:%.*]] = sub i16 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i16 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i16 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = and i16 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = icmp slt i16 [[TMP50]], 0
; CHECK:    [[TMP52:%.*]] = icmp slt i16 [[TMP45]], 0
; CHECK:    [[TMP53:%.*]] = select i1 [[TMP52]], i16 -32768, i16 32767
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP51]], i16 [[TMP53]], i16 [[TMP47]]
; CHECK:    [[TMP55:%.*]] = insertelement <8 x i16> [[TMP44]], i16 [[TMP54]], i64 4
; CHECK:    [[TMP56:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP57:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP58:%.*]] = sub i16 [[TMP56]], [[TMP57]]
; CHECK:    [[TMP59:%.*]] = xor i16 [[TMP56]], [[TMP57]]
; CHECK:    [[TMP60:%.*]] = xor i16 [[TMP58]], [[TMP56]]
; CHECK:    [[TMP61:%.*]] = and i16 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP62:%.*]] = icmp slt i16 [[TMP61]], 0
; CHECK:    [[TMP63:%.*]] = icmp slt i16 [[TMP56]], 0
; CHECK:    [[TMP64:%.*]] = select i1 [[TMP63]], i16 -32768, i16 32767
; CHECK:    [[TMP65:%.*]] = select i1 [[TMP62]], i16 [[TMP64]], i16 [[TMP58]]
; CHECK:    [[TMP66:%.*]] = insertelement <8 x i16> [[TMP55]], i16 [[TMP65]], i64 5
; CHECK:    [[TMP67:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP68:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP69:%.*]] = sub i16 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP70:%.*]] = xor i16 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP71:%.*]] = xor i16 [[TMP69]], [[TMP67]]
; CHECK:    [[TMP72:%.*]] = and i16 [[TMP70]], [[TMP71]]
; CHECK:    [[TMP73:%.*]] = icmp slt i16 [[TMP72]], 0
; CHECK:    [[TMP74:%.*]] = icmp slt i16 [[TMP67]], 0
; CHECK:    [[TMP75:%.*]] = select i1 [[TMP74]], i16 -32768, i16 32767
; CHECK:    [[TMP76:%.*]] = select i1 [[TMP73]], i16 [[TMP75]], i16 [[TMP69]]
; CHECK:    [[TMP77:%.*]] = insertelement <8 x i16> [[TMP66]], i16 [[TMP76]], i64 6
; CHECK:    [[TMP78:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP79:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP80:%.*]] = sub i16 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP81:%.*]] = xor i16 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP82:%.*]] = xor i16 [[TMP80]], [[TMP78]]
; CHECK:    [[TMP83:%.*]] = and i16 [[TMP81]], [[TMP82]]
; CHECK:    [[TMP84:%.*]] = icmp slt i16 [[TMP83]], 0
; CHECK:    [[TMP85:%.*]] = icmp slt i16 [[TMP78]], 0
; CHECK:    [[TMP86:%.*]] = select i1 [[TMP85]], i16 -32768, i16 32767
; CHECK:    [[TMP87:%.*]] = select i1 [[TMP84]], i16 [[TMP86]], i16 [[TMP80]]
; CHECK:    [[TMP88:%.*]] = insertelement <8 x i16> [[TMP77]], i16 [[TMP87]], i64 7
; CHECK:    ret <8 x i16> [[TMP88]]
;
  %1 = call <8 x i16> @llvm.ssub.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_sadd_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_sadd_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <8 x i16> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i16> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i16 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i16 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i16 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt i16 [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = icmp slt i16 [[TMP1]], 0
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i16 -32768, i16 32767
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP8]], i16 [[TMP10]], i16 [[TMP3]]
; CHECK:    [[TMP12:%.*]] = insertelement <8 x i16> poison, i16 [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP14:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP15:%.*]] = add i16 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = xor i16 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP17:%.*]] = xor i16 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP18:%.*]] = xor i16 [[TMP16]], -1
; CHECK:    [[TMP19:%.*]] = and i16 [[TMP18]], [[TMP17]]
; CHECK:    [[TMP20:%.*]] = icmp slt i16 [[TMP19]], 0
; CHECK:    [[TMP21:%.*]] = icmp slt i16 [[TMP13]], 0
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP21]], i16 -32768, i16 32767
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP20]], i16 [[TMP22]], i16 [[TMP15]]
; CHECK:    [[TMP24:%.*]] = insertelement <8 x i16> [[TMP12]], i16 [[TMP23]], i64 1
; CHECK:    [[TMP25:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP26:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP27:%.*]] = add i16 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = xor i16 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP29:%.*]] = xor i16 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP30:%.*]] = xor i16 [[TMP28]], -1
; CHECK:    [[TMP31:%.*]] = and i16 [[TMP30]], [[TMP29]]
; CHECK:    [[TMP32:%.*]] = icmp slt i16 [[TMP31]], 0
; CHECK:    [[TMP33:%.*]] = icmp slt i16 [[TMP25]], 0
; CHECK:    [[TMP34:%.*]] = select i1 [[TMP33]], i16 -32768, i16 32767
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP32]], i16 [[TMP34]], i16 [[TMP27]]
; CHECK:    [[TMP36:%.*]] = insertelement <8 x i16> [[TMP24]], i16 [[TMP35]], i64 2
; CHECK:    [[TMP37:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP38:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP39:%.*]] = add i16 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = xor i16 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = xor i16 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP42:%.*]] = xor i16 [[TMP40]], -1
; CHECK:    [[TMP43:%.*]] = and i16 [[TMP42]], [[TMP41]]
; CHECK:    [[TMP44:%.*]] = icmp slt i16 [[TMP43]], 0
; CHECK:    [[TMP45:%.*]] = icmp slt i16 [[TMP37]], 0
; CHECK:    [[TMP46:%.*]] = select i1 [[TMP45]], i16 -32768, i16 32767
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP44]], i16 [[TMP46]], i16 [[TMP39]]
; CHECK:    [[TMP48:%.*]] = insertelement <8 x i16> [[TMP36]], i16 [[TMP47]], i64 3
; CHECK:    [[TMP49:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP50:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP51:%.*]] = add i16 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP52:%.*]] = xor i16 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP53:%.*]] = xor i16 [[TMP51]], [[TMP49]]
; CHECK:    [[TMP54:%.*]] = xor i16 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i16 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i16 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i16 [[TMP49]], 0
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i16 -32768, i16 32767
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i16 [[TMP58]], i16 [[TMP51]]
; CHECK:    [[TMP60:%.*]] = insertelement <8 x i16> [[TMP48]], i16 [[TMP59]], i64 4
; CHECK:    [[TMP61:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP62:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP63:%.*]] = add i16 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP64:%.*]] = xor i16 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP65:%.*]] = xor i16 [[TMP63]], [[TMP61]]
; CHECK:    [[TMP66:%.*]] = xor i16 [[TMP64]], -1
; CHECK:    [[TMP67:%.*]] = and i16 [[TMP66]], [[TMP65]]
; CHECK:    [[TMP68:%.*]] = icmp slt i16 [[TMP67]], 0
; CHECK:    [[TMP69:%.*]] = icmp slt i16 [[TMP61]], 0
; CHECK:    [[TMP70:%.*]] = select i1 [[TMP69]], i16 -32768, i16 32767
; CHECK:    [[TMP71:%.*]] = select i1 [[TMP68]], i16 [[TMP70]], i16 [[TMP63]]
; CHECK:    [[TMP72:%.*]] = insertelement <8 x i16> [[TMP60]], i16 [[TMP71]], i64 5
; CHECK:    [[TMP73:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP74:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP75:%.*]] = add i16 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = xor i16 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP77:%.*]] = xor i16 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP78:%.*]] = xor i16 [[TMP76]], -1
; CHECK:    [[TMP79:%.*]] = and i16 [[TMP78]], [[TMP77]]
; CHECK:    [[TMP80:%.*]] = icmp slt i16 [[TMP79]], 0
; CHECK:    [[TMP81:%.*]] = icmp slt i16 [[TMP73]], 0
; CHECK:    [[TMP82:%.*]] = select i1 [[TMP81]], i16 -32768, i16 32767
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP80]], i16 [[TMP82]], i16 [[TMP75]]
; CHECK:    [[TMP84:%.*]] = insertelement <8 x i16> [[TMP72]], i16 [[TMP83]], i64 6
; CHECK:    [[TMP85:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP86:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP87:%.*]] = add i16 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP88:%.*]] = xor i16 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP89:%.*]] = xor i16 [[TMP87]], [[TMP85]]
; CHECK:    [[TMP90:%.*]] = xor i16 [[TMP88]], -1
; CHECK:    [[TMP91:%.*]] = and i16 [[TMP90]], [[TMP89]]
; CHECK:    [[TMP92:%.*]] = icmp slt i16 [[TMP91]], 0
; CHECK:    [[TMP93:%.*]] = icmp slt i16 [[TMP85]], 0
; CHECK:    [[TMP94:%.*]] = select i1 [[TMP93]], i16 -32768, i16 32767
; CHECK:    [[TMP95:%.*]] = select i1 [[TMP92]], i16 [[TMP94]], i16 [[TMP87]]
; CHECK:    [[TMP96:%.*]] = insertelement <8 x i16> [[TMP84]], i16 [[TMP95]], i64 7
; CHECK:    ret <8 x i16> [[TMP96]]
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
; CHECK:    [[TMP1:%.*]] = extractelement <16 x i8> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ugt i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i8 0, i8 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <16 x i8> poison, i8 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = sub i8 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ugt i8 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i8 0, i8 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <16 x i8> [[TMP6]], i8 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = sub i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ugt i8 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i8 0, i8 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <16 x i8> [[TMP12]], i8 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = sub i8 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ugt i8 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i8 0, i8 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <16 x i8> [[TMP18]], i8 [[TMP23]], i64 3
; CHECK:    [[TMP25:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP26:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP27:%.*]] = sub i8 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = icmp ugt i8 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP28]], i8 0, i8 [[TMP27]]
; CHECK:    [[TMP30:%.*]] = insertelement <16 x i8> [[TMP24]], i8 [[TMP29]], i64 4
; CHECK:    [[TMP31:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP32:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP33:%.*]] = sub i8 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = icmp ugt i8 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP34]], i8 0, i8 [[TMP33]]
; CHECK:    [[TMP36:%.*]] = insertelement <16 x i8> [[TMP30]], i8 [[TMP35]], i64 5
; CHECK:    [[TMP37:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP38:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP39:%.*]] = sub i8 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp ugt i8 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP40]], i8 0, i8 [[TMP39]]
; CHECK:    [[TMP42:%.*]] = insertelement <16 x i8> [[TMP36]], i8 [[TMP41]], i64 6
; CHECK:    [[TMP43:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP44:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP45:%.*]] = sub i8 [[TMP43]], [[TMP44]]
; CHECK:    [[TMP46:%.*]] = icmp ugt i8 [[TMP45]], [[TMP43]]
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP46]], i8 0, i8 [[TMP45]]
; CHECK:    [[TMP48:%.*]] = insertelement <16 x i8> [[TMP42]], i8 [[TMP47]], i64 7
; CHECK:    [[TMP49:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP50:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP51:%.*]] = sub i8 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP52:%.*]] = icmp ugt i8 [[TMP51]], [[TMP49]]
; CHECK:    [[TMP53:%.*]] = select i1 [[TMP52]], i8 0, i8 [[TMP51]]
; CHECK:    [[TMP54:%.*]] = insertelement <16 x i8> [[TMP48]], i8 [[TMP53]], i64 8
; CHECK:    [[TMP55:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP56:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP57:%.*]] = sub i8 [[TMP55]], [[TMP56]]
; CHECK:    [[TMP58:%.*]] = icmp ugt i8 [[TMP57]], [[TMP55]]
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP58]], i8 0, i8 [[TMP57]]
; CHECK:    [[TMP60:%.*]] = insertelement <16 x i8> [[TMP54]], i8 [[TMP59]], i64 9
; CHECK:    [[TMP61:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP62:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP63:%.*]] = sub i8 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP64:%.*]] = icmp ugt i8 [[TMP63]], [[TMP61]]
; CHECK:    [[TMP65:%.*]] = select i1 [[TMP64]], i8 0, i8 [[TMP63]]
; CHECK:    [[TMP66:%.*]] = insertelement <16 x i8> [[TMP60]], i8 [[TMP65]], i64 10
; CHECK:    [[TMP67:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP68:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP69:%.*]] = sub i8 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP70:%.*]] = icmp ugt i8 [[TMP69]], [[TMP67]]
; CHECK:    [[TMP71:%.*]] = select i1 [[TMP70]], i8 0, i8 [[TMP69]]
; CHECK:    [[TMP72:%.*]] = insertelement <16 x i8> [[TMP66]], i8 [[TMP71]], i64 11
; CHECK:    [[TMP73:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP74:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP75:%.*]] = sub i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = icmp ugt i8 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP77:%.*]] = select i1 [[TMP76]], i8 0, i8 [[TMP75]]
; CHECK:    [[TMP78:%.*]] = insertelement <16 x i8> [[TMP72]], i8 [[TMP77]], i64 12
; CHECK:    [[TMP79:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP80:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP81:%.*]] = sub i8 [[TMP79]], [[TMP80]]
; CHECK:    [[TMP82:%.*]] = icmp ugt i8 [[TMP81]], [[TMP79]]
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP82]], i8 0, i8 [[TMP81]]
; CHECK:    [[TMP84:%.*]] = insertelement <16 x i8> [[TMP78]], i8 [[TMP83]], i64 13
; CHECK:    [[TMP85:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP86:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP87:%.*]] = sub i8 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP88:%.*]] = icmp ugt i8 [[TMP87]], [[TMP85]]
; CHECK:    [[TMP89:%.*]] = select i1 [[TMP88]], i8 0, i8 [[TMP87]]
; CHECK:    [[TMP90:%.*]] = insertelement <16 x i8> [[TMP84]], i8 [[TMP89]], i64 14
; CHECK:    [[TMP91:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP92:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP93:%.*]] = sub i8 [[TMP91]], [[TMP92]]
; CHECK:    [[TMP94:%.*]] = icmp ugt i8 [[TMP93]], [[TMP91]]
; CHECK:    [[TMP95:%.*]] = select i1 [[TMP94]], i8 0, i8 [[TMP93]]
; CHECK:    [[TMP96:%.*]] = insertelement <16 x i8> [[TMP90]], i8 [[TMP95]], i64 15
; CHECK:    ret <16 x i8> [[TMP96]]
;
  %1 = call <16 x i8> @llvm.usub.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_uadd_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_uadd_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <16 x i8> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp ult i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP4]], i8 -1, i8 [[TMP3]]
; CHECK:    [[TMP6:%.*]] = insertelement <16 x i8> poison, i8 [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP9:%.*]] = add i8 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = icmp ult i8 [[TMP9]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i8 -1, i8 [[TMP9]]
; CHECK:    [[TMP12:%.*]] = insertelement <16 x i8> [[TMP6]], i8 [[TMP11]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP14:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP15:%.*]] = add i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = icmp ult i8 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i8 -1, i8 [[TMP15]]
; CHECK:    [[TMP18:%.*]] = insertelement <16 x i8> [[TMP12]], i8 [[TMP17]], i64 2
; CHECK:    [[TMP19:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP20:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP21:%.*]] = add i8 [[TMP19]], [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ult i8 [[TMP21]], [[TMP19]]
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i8 -1, i8 [[TMP21]]
; CHECK:    [[TMP24:%.*]] = insertelement <16 x i8> [[TMP18]], i8 [[TMP23]], i64 3
; CHECK:    [[TMP25:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP26:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP27:%.*]] = add i8 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = icmp ult i8 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP28]], i8 -1, i8 [[TMP27]]
; CHECK:    [[TMP30:%.*]] = insertelement <16 x i8> [[TMP24]], i8 [[TMP29]], i64 4
; CHECK:    [[TMP31:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP32:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP33:%.*]] = add i8 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = icmp ult i8 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP34]], i8 -1, i8 [[TMP33]]
; CHECK:    [[TMP36:%.*]] = insertelement <16 x i8> [[TMP30]], i8 [[TMP35]], i64 5
; CHECK:    [[TMP37:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP38:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP39:%.*]] = add i8 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp ult i8 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP40]], i8 -1, i8 [[TMP39]]
; CHECK:    [[TMP42:%.*]] = insertelement <16 x i8> [[TMP36]], i8 [[TMP41]], i64 6
; CHECK:    [[TMP43:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP44:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP45:%.*]] = add i8 [[TMP43]], [[TMP44]]
; CHECK:    [[TMP46:%.*]] = icmp ult i8 [[TMP45]], [[TMP43]]
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP46]], i8 -1, i8 [[TMP45]]
; CHECK:    [[TMP48:%.*]] = insertelement <16 x i8> [[TMP42]], i8 [[TMP47]], i64 7
; CHECK:    [[TMP49:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP50:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP51:%.*]] = add i8 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP52:%.*]] = icmp ult i8 [[TMP51]], [[TMP49]]
; CHECK:    [[TMP53:%.*]] = select i1 [[TMP52]], i8 -1, i8 [[TMP51]]
; CHECK:    [[TMP54:%.*]] = insertelement <16 x i8> [[TMP48]], i8 [[TMP53]], i64 8
; CHECK:    [[TMP55:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP56:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP57:%.*]] = add i8 [[TMP55]], [[TMP56]]
; CHECK:    [[TMP58:%.*]] = icmp ult i8 [[TMP57]], [[TMP55]]
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP58]], i8 -1, i8 [[TMP57]]
; CHECK:    [[TMP60:%.*]] = insertelement <16 x i8> [[TMP54]], i8 [[TMP59]], i64 9
; CHECK:    [[TMP61:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP62:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP63:%.*]] = add i8 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP64:%.*]] = icmp ult i8 [[TMP63]], [[TMP61]]
; CHECK:    [[TMP65:%.*]] = select i1 [[TMP64]], i8 -1, i8 [[TMP63]]
; CHECK:    [[TMP66:%.*]] = insertelement <16 x i8> [[TMP60]], i8 [[TMP65]], i64 10
; CHECK:    [[TMP67:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP68:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP69:%.*]] = add i8 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP70:%.*]] = icmp ult i8 [[TMP69]], [[TMP67]]
; CHECK:    [[TMP71:%.*]] = select i1 [[TMP70]], i8 -1, i8 [[TMP69]]
; CHECK:    [[TMP72:%.*]] = insertelement <16 x i8> [[TMP66]], i8 [[TMP71]], i64 11
; CHECK:    [[TMP73:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP74:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP75:%.*]] = add i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = icmp ult i8 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP77:%.*]] = select i1 [[TMP76]], i8 -1, i8 [[TMP75]]
; CHECK:    [[TMP78:%.*]] = insertelement <16 x i8> [[TMP72]], i8 [[TMP77]], i64 12
; CHECK:    [[TMP79:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP80:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP81:%.*]] = add i8 [[TMP79]], [[TMP80]]
; CHECK:    [[TMP82:%.*]] = icmp ult i8 [[TMP81]], [[TMP79]]
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP82]], i8 -1, i8 [[TMP81]]
; CHECK:    [[TMP84:%.*]] = insertelement <16 x i8> [[TMP78]], i8 [[TMP83]], i64 13
; CHECK:    [[TMP85:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP86:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP87:%.*]] = add i8 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP88:%.*]] = icmp ult i8 [[TMP87]], [[TMP85]]
; CHECK:    [[TMP89:%.*]] = select i1 [[TMP88]], i8 -1, i8 [[TMP87]]
; CHECK:    [[TMP90:%.*]] = insertelement <16 x i8> [[TMP84]], i8 [[TMP89]], i64 14
; CHECK:    [[TMP91:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP92:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP93:%.*]] = add i8 [[TMP91]], [[TMP92]]
; CHECK:    [[TMP94:%.*]] = icmp ult i8 [[TMP93]], [[TMP91]]
; CHECK:    [[TMP95:%.*]] = select i1 [[TMP94]], i8 -1, i8 [[TMP93]]
; CHECK:    [[TMP96:%.*]] = insertelement <16 x i8> [[TMP90]], i8 [[TMP95]], i64 15
; CHECK:    ret <16 x i8> [[TMP96]]
;
  %1 = call <16 x i8> @llvm.uadd.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_sadd_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_sadd_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <16 x i8> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = add i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = xor i8 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = and i8 [[TMP6]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp slt i8 [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = icmp slt i8 [[TMP1]], 0
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP9]], i8 -128, i8 127
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP8]], i8 [[TMP10]], i8 [[TMP3]]
; CHECK:    [[TMP12:%.*]] = insertelement <16 x i8> poison, i8 [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP14:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP15:%.*]] = add i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = xor i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP17:%.*]] = xor i8 [[TMP15]], [[TMP13]]
; CHECK:    [[TMP18:%.*]] = xor i8 [[TMP16]], -1
; CHECK:    [[TMP19:%.*]] = and i8 [[TMP18]], [[TMP17]]
; CHECK:    [[TMP20:%.*]] = icmp slt i8 [[TMP19]], 0
; CHECK:    [[TMP21:%.*]] = icmp slt i8 [[TMP13]], 0
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP21]], i8 -128, i8 127
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP20]], i8 [[TMP22]], i8 [[TMP15]]
; CHECK:    [[TMP24:%.*]] = insertelement <16 x i8> [[TMP12]], i8 [[TMP23]], i64 1
; CHECK:    [[TMP25:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP26:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP27:%.*]] = add i8 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP28:%.*]] = xor i8 [[TMP25]], [[TMP26]]
; CHECK:    [[TMP29:%.*]] = xor i8 [[TMP27]], [[TMP25]]
; CHECK:    [[TMP30:%.*]] = xor i8 [[TMP28]], -1
; CHECK:    [[TMP31:%.*]] = and i8 [[TMP30]], [[TMP29]]
; CHECK:    [[TMP32:%.*]] = icmp slt i8 [[TMP31]], 0
; CHECK:    [[TMP33:%.*]] = icmp slt i8 [[TMP25]], 0
; CHECK:    [[TMP34:%.*]] = select i1 [[TMP33]], i8 -128, i8 127
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP32]], i8 [[TMP34]], i8 [[TMP27]]
; CHECK:    [[TMP36:%.*]] = insertelement <16 x i8> [[TMP24]], i8 [[TMP35]], i64 2
; CHECK:    [[TMP37:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP38:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP39:%.*]] = add i8 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = xor i8 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = xor i8 [[TMP39]], [[TMP37]]
; CHECK:    [[TMP42:%.*]] = xor i8 [[TMP40]], -1
; CHECK:    [[TMP43:%.*]] = and i8 [[TMP42]], [[TMP41]]
; CHECK:    [[TMP44:%.*]] = icmp slt i8 [[TMP43]], 0
; CHECK:    [[TMP45:%.*]] = icmp slt i8 [[TMP37]], 0
; CHECK:    [[TMP46:%.*]] = select i1 [[TMP45]], i8 -128, i8 127
; CHECK:    [[TMP47:%.*]] = select i1 [[TMP44]], i8 [[TMP46]], i8 [[TMP39]]
; CHECK:    [[TMP48:%.*]] = insertelement <16 x i8> [[TMP36]], i8 [[TMP47]], i64 3
; CHECK:    [[TMP49:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP50:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP51:%.*]] = add i8 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP52:%.*]] = xor i8 [[TMP49]], [[TMP50]]
; CHECK:    [[TMP53:%.*]] = xor i8 [[TMP51]], [[TMP49]]
; CHECK:    [[TMP54:%.*]] = xor i8 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i8 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i8 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i8 [[TMP49]], 0
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i8 -128, i8 127
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i8 [[TMP58]], i8 [[TMP51]]
; CHECK:    [[TMP60:%.*]] = insertelement <16 x i8> [[TMP48]], i8 [[TMP59]], i64 4
; CHECK:    [[TMP61:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP62:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP63:%.*]] = add i8 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP64:%.*]] = xor i8 [[TMP61]], [[TMP62]]
; CHECK:    [[TMP65:%.*]] = xor i8 [[TMP63]], [[TMP61]]
; CHECK:    [[TMP66:%.*]] = xor i8 [[TMP64]], -1
; CHECK:    [[TMP67:%.*]] = and i8 [[TMP66]], [[TMP65]]
; CHECK:    [[TMP68:%.*]] = icmp slt i8 [[TMP67]], 0
; CHECK:    [[TMP69:%.*]] = icmp slt i8 [[TMP61]], 0
; CHECK:    [[TMP70:%.*]] = select i1 [[TMP69]], i8 -128, i8 127
; CHECK:    [[TMP71:%.*]] = select i1 [[TMP68]], i8 [[TMP70]], i8 [[TMP63]]
; CHECK:    [[TMP72:%.*]] = insertelement <16 x i8> [[TMP60]], i8 [[TMP71]], i64 5
; CHECK:    [[TMP73:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP74:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP75:%.*]] = add i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = xor i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP77:%.*]] = xor i8 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP78:%.*]] = xor i8 [[TMP76]], -1
; CHECK:    [[TMP79:%.*]] = and i8 [[TMP78]], [[TMP77]]
; CHECK:    [[TMP80:%.*]] = icmp slt i8 [[TMP79]], 0
; CHECK:    [[TMP81:%.*]] = icmp slt i8 [[TMP73]], 0
; CHECK:    [[TMP82:%.*]] = select i1 [[TMP81]], i8 -128, i8 127
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP80]], i8 [[TMP82]], i8 [[TMP75]]
; CHECK:    [[TMP84:%.*]] = insertelement <16 x i8> [[TMP72]], i8 [[TMP83]], i64 6
; CHECK:    [[TMP85:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP86:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP87:%.*]] = add i8 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP88:%.*]] = xor i8 [[TMP85]], [[TMP86]]
; CHECK:    [[TMP89:%.*]] = xor i8 [[TMP87]], [[TMP85]]
; CHECK:    [[TMP90:%.*]] = xor i8 [[TMP88]], -1
; CHECK:    [[TMP91:%.*]] = and i8 [[TMP90]], [[TMP89]]
; CHECK:    [[TMP92:%.*]] = icmp slt i8 [[TMP91]], 0
; CHECK:    [[TMP93:%.*]] = icmp slt i8 [[TMP85]], 0
; CHECK:    [[TMP94:%.*]] = select i1 [[TMP93]], i8 -128, i8 127
; CHECK:    [[TMP95:%.*]] = select i1 [[TMP92]], i8 [[TMP94]], i8 [[TMP87]]
; CHECK:    [[TMP96:%.*]] = insertelement <16 x i8> [[TMP84]], i8 [[TMP95]], i64 7
; CHECK:    [[TMP97:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP98:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP99:%.*]] = add i8 [[TMP97]], [[TMP98]]
; CHECK:    [[TMP100:%.*]] = xor i8 [[TMP97]], [[TMP98]]
; CHECK:    [[TMP101:%.*]] = xor i8 [[TMP99]], [[TMP97]]
; CHECK:    [[TMP102:%.*]] = xor i8 [[TMP100]], -1
; CHECK:    [[TMP103:%.*]] = and i8 [[TMP102]], [[TMP101]]
; CHECK:    [[TMP104:%.*]] = icmp slt i8 [[TMP103]], 0
; CHECK:    [[TMP105:%.*]] = icmp slt i8 [[TMP97]], 0
; CHECK:    [[TMP106:%.*]] = select i1 [[TMP105]], i8 -128, i8 127
; CHECK:    [[TMP107:%.*]] = select i1 [[TMP104]], i8 [[TMP106]], i8 [[TMP99]]
; CHECK:    [[TMP108:%.*]] = insertelement <16 x i8> [[TMP96]], i8 [[TMP107]], i64 8
; CHECK:    [[TMP109:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP110:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP111:%.*]] = add i8 [[TMP109]], [[TMP110]]
; CHECK:    [[TMP112:%.*]] = xor i8 [[TMP109]], [[TMP110]]
; CHECK:    [[TMP113:%.*]] = xor i8 [[TMP111]], [[TMP109]]
; CHECK:    [[TMP114:%.*]] = xor i8 [[TMP112]], -1
; CHECK:    [[TMP115:%.*]] = and i8 [[TMP114]], [[TMP113]]
; CHECK:    [[TMP116:%.*]] = icmp slt i8 [[TMP115]], 0
; CHECK:    [[TMP117:%.*]] = icmp slt i8 [[TMP109]], 0
; CHECK:    [[TMP118:%.*]] = select i1 [[TMP117]], i8 -128, i8 127
; CHECK:    [[TMP119:%.*]] = select i1 [[TMP116]], i8 [[TMP118]], i8 [[TMP111]]
; CHECK:    [[TMP120:%.*]] = insertelement <16 x i8> [[TMP108]], i8 [[TMP119]], i64 9
; CHECK:    [[TMP121:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP122:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP123:%.*]] = add i8 [[TMP121]], [[TMP122]]
; CHECK:    [[TMP124:%.*]] = xor i8 [[TMP121]], [[TMP122]]
; CHECK:    [[TMP125:%.*]] = xor i8 [[TMP123]], [[TMP121]]
; CHECK:    [[TMP126:%.*]] = xor i8 [[TMP124]], -1
; CHECK:    [[TMP127:%.*]] = and i8 [[TMP126]], [[TMP125]]
; CHECK:    [[TMP128:%.*]] = icmp slt i8 [[TMP127]], 0
; CHECK:    [[TMP129:%.*]] = icmp slt i8 [[TMP121]], 0
; CHECK:    [[TMP130:%.*]] = select i1 [[TMP129]], i8 -128, i8 127
; CHECK:    [[TMP131:%.*]] = select i1 [[TMP128]], i8 [[TMP130]], i8 [[TMP123]]
; CHECK:    [[TMP132:%.*]] = insertelement <16 x i8> [[TMP120]], i8 [[TMP131]], i64 10
; CHECK:    [[TMP133:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP134:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP135:%.*]] = add i8 [[TMP133]], [[TMP134]]
; CHECK:    [[TMP136:%.*]] = xor i8 [[TMP133]], [[TMP134]]
; CHECK:    [[TMP137:%.*]] = xor i8 [[TMP135]], [[TMP133]]
; CHECK:    [[TMP138:%.*]] = xor i8 [[TMP136]], -1
; CHECK:    [[TMP139:%.*]] = and i8 [[TMP138]], [[TMP137]]
; CHECK:    [[TMP140:%.*]] = icmp slt i8 [[TMP139]], 0
; CHECK:    [[TMP141:%.*]] = icmp slt i8 [[TMP133]], 0
; CHECK:    [[TMP142:%.*]] = select i1 [[TMP141]], i8 -128, i8 127
; CHECK:    [[TMP143:%.*]] = select i1 [[TMP140]], i8 [[TMP142]], i8 [[TMP135]]
; CHECK:    [[TMP144:%.*]] = insertelement <16 x i8> [[TMP132]], i8 [[TMP143]], i64 11
; CHECK:    [[TMP145:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP146:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP147:%.*]] = add i8 [[TMP145]], [[TMP146]]
; CHECK:    [[TMP148:%.*]] = xor i8 [[TMP145]], [[TMP146]]
; CHECK:    [[TMP149:%.*]] = xor i8 [[TMP147]], [[TMP145]]
; CHECK:    [[TMP150:%.*]] = xor i8 [[TMP148]], -1
; CHECK:    [[TMP151:%.*]] = and i8 [[TMP150]], [[TMP149]]
; CHECK:    [[TMP152:%.*]] = icmp slt i8 [[TMP151]], 0
; CHECK:    [[TMP153:%.*]] = icmp slt i8 [[TMP145]], 0
; CHECK:    [[TMP154:%.*]] = select i1 [[TMP153]], i8 -128, i8 127
; CHECK:    [[TMP155:%.*]] = select i1 [[TMP152]], i8 [[TMP154]], i8 [[TMP147]]
; CHECK:    [[TMP156:%.*]] = insertelement <16 x i8> [[TMP144]], i8 [[TMP155]], i64 12
; CHECK:    [[TMP157:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP158:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP159:%.*]] = add i8 [[TMP157]], [[TMP158]]
; CHECK:    [[TMP160:%.*]] = xor i8 [[TMP157]], [[TMP158]]
; CHECK:    [[TMP161:%.*]] = xor i8 [[TMP159]], [[TMP157]]
; CHECK:    [[TMP162:%.*]] = xor i8 [[TMP160]], -1
; CHECK:    [[TMP163:%.*]] = and i8 [[TMP162]], [[TMP161]]
; CHECK:    [[TMP164:%.*]] = icmp slt i8 [[TMP163]], 0
; CHECK:    [[TMP165:%.*]] = icmp slt i8 [[TMP157]], 0
; CHECK:    [[TMP166:%.*]] = select i1 [[TMP165]], i8 -128, i8 127
; CHECK:    [[TMP167:%.*]] = select i1 [[TMP164]], i8 [[TMP166]], i8 [[TMP159]]
; CHECK:    [[TMP168:%.*]] = insertelement <16 x i8> [[TMP156]], i8 [[TMP167]], i64 13
; CHECK:    [[TMP169:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP170:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP171:%.*]] = add i8 [[TMP169]], [[TMP170]]
; CHECK:    [[TMP172:%.*]] = xor i8 [[TMP169]], [[TMP170]]
; CHECK:    [[TMP173:%.*]] = xor i8 [[TMP171]], [[TMP169]]
; CHECK:    [[TMP174:%.*]] = xor i8 [[TMP172]], -1
; CHECK:    [[TMP175:%.*]] = and i8 [[TMP174]], [[TMP173]]
; CHECK:    [[TMP176:%.*]] = icmp slt i8 [[TMP175]], 0
; CHECK:    [[TMP177:%.*]] = icmp slt i8 [[TMP169]], 0
; CHECK:    [[TMP178:%.*]] = select i1 [[TMP177]], i8 -128, i8 127
; CHECK:    [[TMP179:%.*]] = select i1 [[TMP176]], i8 [[TMP178]], i8 [[TMP171]]
; CHECK:    [[TMP180:%.*]] = insertelement <16 x i8> [[TMP168]], i8 [[TMP179]], i64 14
; CHECK:    [[TMP181:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP182:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP183:%.*]] = add i8 [[TMP181]], [[TMP182]]
; CHECK:    [[TMP184:%.*]] = xor i8 [[TMP181]], [[TMP182]]
; CHECK:    [[TMP185:%.*]] = xor i8 [[TMP183]], [[TMP181]]
; CHECK:    [[TMP186:%.*]] = xor i8 [[TMP184]], -1
; CHECK:    [[TMP187:%.*]] = and i8 [[TMP186]], [[TMP185]]
; CHECK:    [[TMP188:%.*]] = icmp slt i8 [[TMP187]], 0
; CHECK:    [[TMP189:%.*]] = icmp slt i8 [[TMP181]], 0
; CHECK:    [[TMP190:%.*]] = select i1 [[TMP189]], i8 -128, i8 127
; CHECK:    [[TMP191:%.*]] = select i1 [[TMP188]], i8 [[TMP190]], i8 [[TMP183]]
; CHECK:    [[TMP192:%.*]] = insertelement <16 x i8> [[TMP180]], i8 [[TMP191]], i64 15
; CHECK:    ret <16 x i8> [[TMP192]]
;
  %1 = call <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_ssub_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_ssub_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <16 x i8> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = sub i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = xor i8 [[TMP3]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = and i8 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = icmp slt i8 [[TMP6]], 0
; CHECK:    [[TMP8:%.*]] = icmp slt i8 [[TMP1]], 0
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP8]], i8 -128, i8 127
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP7]], i8 [[TMP9]], i8 [[TMP3]]
; CHECK:    [[TMP11:%.*]] = insertelement <16 x i8> poison, i8 [[TMP10]], i64 0
; CHECK:    [[TMP12:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP13:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP14:%.*]] = sub i8 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP15:%.*]] = xor i8 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP16:%.*]] = xor i8 [[TMP14]], [[TMP12]]
; CHECK:    [[TMP17:%.*]] = and i8 [[TMP15]], [[TMP16]]
; CHECK:    [[TMP18:%.*]] = icmp slt i8 [[TMP17]], 0
; CHECK:    [[TMP19:%.*]] = icmp slt i8 [[TMP12]], 0
; CHECK:    [[TMP20:%.*]] = select i1 [[TMP19]], i8 -128, i8 127
; CHECK:    [[TMP21:%.*]] = select i1 [[TMP18]], i8 [[TMP20]], i8 [[TMP14]]
; CHECK:    [[TMP22:%.*]] = insertelement <16 x i8> [[TMP11]], i8 [[TMP21]], i64 1
; CHECK:    [[TMP23:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP24:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP25:%.*]] = sub i8 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP26:%.*]] = xor i8 [[TMP23]], [[TMP24]]
; CHECK:    [[TMP27:%.*]] = xor i8 [[TMP25]], [[TMP23]]
; CHECK:    [[TMP28:%.*]] = and i8 [[TMP26]], [[TMP27]]
; CHECK:    [[TMP29:%.*]] = icmp slt i8 [[TMP28]], 0
; CHECK:    [[TMP30:%.*]] = icmp slt i8 [[TMP23]], 0
; CHECK:    [[TMP31:%.*]] = select i1 [[TMP30]], i8 -128, i8 127
; CHECK:    [[TMP32:%.*]] = select i1 [[TMP29]], i8 [[TMP31]], i8 [[TMP25]]
; CHECK:    [[TMP33:%.*]] = insertelement <16 x i8> [[TMP22]], i8 [[TMP32]], i64 2
; CHECK:    [[TMP34:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP35:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP36:%.*]] = sub i8 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP37:%.*]] = xor i8 [[TMP34]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = xor i8 [[TMP36]], [[TMP34]]
; CHECK:    [[TMP39:%.*]] = and i8 [[TMP37]], [[TMP38]]
; CHECK:    [[TMP40:%.*]] = icmp slt i8 [[TMP39]], 0
; CHECK:    [[TMP41:%.*]] = icmp slt i8 [[TMP34]], 0
; CHECK:    [[TMP42:%.*]] = select i1 [[TMP41]], i8 -128, i8 127
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP40]], i8 [[TMP42]], i8 [[TMP36]]
; CHECK:    [[TMP44:%.*]] = insertelement <16 x i8> [[TMP33]], i8 [[TMP43]], i64 3
; CHECK:    [[TMP45:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP46:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP47:%.*]] = sub i8 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i8 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i8 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = and i8 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = icmp slt i8 [[TMP50]], 0
; CHECK:    [[TMP52:%.*]] = icmp slt i8 [[TMP45]], 0
; CHECK:    [[TMP53:%.*]] = select i1 [[TMP52]], i8 -128, i8 127
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP51]], i8 [[TMP53]], i8 [[TMP47]]
; CHECK:    [[TMP55:%.*]] = insertelement <16 x i8> [[TMP44]], i8 [[TMP54]], i64 4
; CHECK:    [[TMP56:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP57:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP58:%.*]] = sub i8 [[TMP56]], [[TMP57]]
; CHECK:    [[TMP59:%.*]] = xor i8 [[TMP56]], [[TMP57]]
; CHECK:    [[TMP60:%.*]] = xor i8 [[TMP58]], [[TMP56]]
; CHECK:    [[TMP61:%.*]] = and i8 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP62:%.*]] = icmp slt i8 [[TMP61]], 0
; CHECK:    [[TMP63:%.*]] = icmp slt i8 [[TMP56]], 0
; CHECK:    [[TMP64:%.*]] = select i1 [[TMP63]], i8 -128, i8 127
; CHECK:    [[TMP65:%.*]] = select i1 [[TMP62]], i8 [[TMP64]], i8 [[TMP58]]
; CHECK:    [[TMP66:%.*]] = insertelement <16 x i8> [[TMP55]], i8 [[TMP65]], i64 5
; CHECK:    [[TMP67:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP68:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP69:%.*]] = sub i8 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP70:%.*]] = xor i8 [[TMP67]], [[TMP68]]
; CHECK:    [[TMP71:%.*]] = xor i8 [[TMP69]], [[TMP67]]
; CHECK:    [[TMP72:%.*]] = and i8 [[TMP70]], [[TMP71]]
; CHECK:    [[TMP73:%.*]] = icmp slt i8 [[TMP72]], 0
; CHECK:    [[TMP74:%.*]] = icmp slt i8 [[TMP67]], 0
; CHECK:    [[TMP75:%.*]] = select i1 [[TMP74]], i8 -128, i8 127
; CHECK:    [[TMP76:%.*]] = select i1 [[TMP73]], i8 [[TMP75]], i8 [[TMP69]]
; CHECK:    [[TMP77:%.*]] = insertelement <16 x i8> [[TMP66]], i8 [[TMP76]], i64 6
; CHECK:    [[TMP78:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP79:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP80:%.*]] = sub i8 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP81:%.*]] = xor i8 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP82:%.*]] = xor i8 [[TMP80]], [[TMP78]]
; CHECK:    [[TMP83:%.*]] = and i8 [[TMP81]], [[TMP82]]
; CHECK:    [[TMP84:%.*]] = icmp slt i8 [[TMP83]], 0
; CHECK:    [[TMP85:%.*]] = icmp slt i8 [[TMP78]], 0
; CHECK:    [[TMP86:%.*]] = select i1 [[TMP85]], i8 -128, i8 127
; CHECK:    [[TMP87:%.*]] = select i1 [[TMP84]], i8 [[TMP86]], i8 [[TMP80]]
; CHECK:    [[TMP88:%.*]] = insertelement <16 x i8> [[TMP77]], i8 [[TMP87]], i64 7
; CHECK:    [[TMP89:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP90:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP91:%.*]] = sub i8 [[TMP89]], [[TMP90]]
; CHECK:    [[TMP92:%.*]] = xor i8 [[TMP89]], [[TMP90]]
; CHECK:    [[TMP93:%.*]] = xor i8 [[TMP91]], [[TMP89]]
; CHECK:    [[TMP94:%.*]] = and i8 [[TMP92]], [[TMP93]]
; CHECK:    [[TMP95:%.*]] = icmp slt i8 [[TMP94]], 0
; CHECK:    [[TMP96:%.*]] = icmp slt i8 [[TMP89]], 0
; CHECK:    [[TMP97:%.*]] = select i1 [[TMP96]], i8 -128, i8 127
; CHECK:    [[TMP98:%.*]] = select i1 [[TMP95]], i8 [[TMP97]], i8 [[TMP91]]
; CHECK:    [[TMP99:%.*]] = insertelement <16 x i8> [[TMP88]], i8 [[TMP98]], i64 8
; CHECK:    [[TMP100:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP101:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP102:%.*]] = sub i8 [[TMP100]], [[TMP101]]
; CHECK:    [[TMP103:%.*]] = xor i8 [[TMP100]], [[TMP101]]
; CHECK:    [[TMP104:%.*]] = xor i8 [[TMP102]], [[TMP100]]
; CHECK:    [[TMP105:%.*]] = and i8 [[TMP103]], [[TMP104]]
; CHECK:    [[TMP106:%.*]] = icmp slt i8 [[TMP105]], 0
; CHECK:    [[TMP107:%.*]] = icmp slt i8 [[TMP100]], 0
; CHECK:    [[TMP108:%.*]] = select i1 [[TMP107]], i8 -128, i8 127
; CHECK:    [[TMP109:%.*]] = select i1 [[TMP106]], i8 [[TMP108]], i8 [[TMP102]]
; CHECK:    [[TMP110:%.*]] = insertelement <16 x i8> [[TMP99]], i8 [[TMP109]], i64 9
; CHECK:    [[TMP111:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP112:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP113:%.*]] = sub i8 [[TMP111]], [[TMP112]]
; CHECK:    [[TMP114:%.*]] = xor i8 [[TMP111]], [[TMP112]]
; CHECK:    [[TMP115:%.*]] = xor i8 [[TMP113]], [[TMP111]]
; CHECK:    [[TMP116:%.*]] = and i8 [[TMP114]], [[TMP115]]
; CHECK:    [[TMP117:%.*]] = icmp slt i8 [[TMP116]], 0
; CHECK:    [[TMP118:%.*]] = icmp slt i8 [[TMP111]], 0
; CHECK:    [[TMP119:%.*]] = select i1 [[TMP118]], i8 -128, i8 127
; CHECK:    [[TMP120:%.*]] = select i1 [[TMP117]], i8 [[TMP119]], i8 [[TMP113]]
; CHECK:    [[TMP121:%.*]] = insertelement <16 x i8> [[TMP110]], i8 [[TMP120]], i64 10
; CHECK:    [[TMP122:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP123:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP124:%.*]] = sub i8 [[TMP122]], [[TMP123]]
; CHECK:    [[TMP125:%.*]] = xor i8 [[TMP122]], [[TMP123]]
; CHECK:    [[TMP126:%.*]] = xor i8 [[TMP124]], [[TMP122]]
; CHECK:    [[TMP127:%.*]] = and i8 [[TMP125]], [[TMP126]]
; CHECK:    [[TMP128:%.*]] = icmp slt i8 [[TMP127]], 0
; CHECK:    [[TMP129:%.*]] = icmp slt i8 [[TMP122]], 0
; CHECK:    [[TMP130:%.*]] = select i1 [[TMP129]], i8 -128, i8 127
; CHECK:    [[TMP131:%.*]] = select i1 [[TMP128]], i8 [[TMP130]], i8 [[TMP124]]
; CHECK:    [[TMP132:%.*]] = insertelement <16 x i8> [[TMP121]], i8 [[TMP131]], i64 11
; CHECK:    [[TMP133:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP134:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP135:%.*]] = sub i8 [[TMP133]], [[TMP134]]
; CHECK:    [[TMP136:%.*]] = xor i8 [[TMP133]], [[TMP134]]
; CHECK:    [[TMP137:%.*]] = xor i8 [[TMP135]], [[TMP133]]
; CHECK:    [[TMP138:%.*]] = and i8 [[TMP136]], [[TMP137]]
; CHECK:    [[TMP139:%.*]] = icmp slt i8 [[TMP138]], 0
; CHECK:    [[TMP140:%.*]] = icmp slt i8 [[TMP133]], 0
; CHECK:    [[TMP141:%.*]] = select i1 [[TMP140]], i8 -128, i8 127
; CHECK:    [[TMP142:%.*]] = select i1 [[TMP139]], i8 [[TMP141]], i8 [[TMP135]]
; CHECK:    [[TMP143:%.*]] = insertelement <16 x i8> [[TMP132]], i8 [[TMP142]], i64 12
; CHECK:    [[TMP144:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP145:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP146:%.*]] = sub i8 [[TMP144]], [[TMP145]]
; CHECK:    [[TMP147:%.*]] = xor i8 [[TMP144]], [[TMP145]]
; CHECK:    [[TMP148:%.*]] = xor i8 [[TMP146]], [[TMP144]]
; CHECK:    [[TMP149:%.*]] = and i8 [[TMP147]], [[TMP148]]
; CHECK:    [[TMP150:%.*]] = icmp slt i8 [[TMP149]], 0
; CHECK:    [[TMP151:%.*]] = icmp slt i8 [[TMP144]], 0
; CHECK:    [[TMP152:%.*]] = select i1 [[TMP151]], i8 -128, i8 127
; CHECK:    [[TMP153:%.*]] = select i1 [[TMP150]], i8 [[TMP152]], i8 [[TMP146]]
; CHECK:    [[TMP154:%.*]] = insertelement <16 x i8> [[TMP143]], i8 [[TMP153]], i64 13
; CHECK:    [[TMP155:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP156:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP157:%.*]] = sub i8 [[TMP155]], [[TMP156]]
; CHECK:    [[TMP158:%.*]] = xor i8 [[TMP155]], [[TMP156]]
; CHECK:    [[TMP159:%.*]] = xor i8 [[TMP157]], [[TMP155]]
; CHECK:    [[TMP160:%.*]] = and i8 [[TMP158]], [[TMP159]]
; CHECK:    [[TMP161:%.*]] = icmp slt i8 [[TMP160]], 0
; CHECK:    [[TMP162:%.*]] = icmp slt i8 [[TMP155]], 0
; CHECK:    [[TMP163:%.*]] = select i1 [[TMP162]], i8 -128, i8 127
; CHECK:    [[TMP164:%.*]] = select i1 [[TMP161]], i8 [[TMP163]], i8 [[TMP157]]
; CHECK:    [[TMP165:%.*]] = insertelement <16 x i8> [[TMP154]], i8 [[TMP164]], i64 14
; CHECK:    [[TMP166:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP167:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP168:%.*]] = sub i8 [[TMP166]], [[TMP167]]
; CHECK:    [[TMP169:%.*]] = xor i8 [[TMP166]], [[TMP167]]
; CHECK:    [[TMP170:%.*]] = xor i8 [[TMP168]], [[TMP166]]
; CHECK:    [[TMP171:%.*]] = and i8 [[TMP169]], [[TMP170]]
; CHECK:    [[TMP172:%.*]] = icmp slt i8 [[TMP171]], 0
; CHECK:    [[TMP173:%.*]] = icmp slt i8 [[TMP166]], 0
; CHECK:    [[TMP174:%.*]] = select i1 [[TMP173]], i8 -128, i8 127
; CHECK:    [[TMP175:%.*]] = select i1 [[TMP172]], i8 [[TMP174]], i8 [[TMP168]]
; CHECK:    [[TMP176:%.*]] = insertelement <16 x i8> [[TMP165]], i8 [[TMP175]], i64 15
; CHECK:    ret <16 x i8> [[TMP176]]
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
