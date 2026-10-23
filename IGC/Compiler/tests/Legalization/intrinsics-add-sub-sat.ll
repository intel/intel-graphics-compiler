;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; TODO: remove nop bitcasts from signed sat intrinsics
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
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP1]] to i64
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[TMP2]] to i64
; CHECK:    [[TMP5:%.*]] = sub i64 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i64 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = xor i64 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = xor i64 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP9:%.*]] = xor i64 [[TMP7]], -1
; CHECK:    [[TMP10:%.*]] = and i64 [[TMP9]], [[TMP8]]
; CHECK:    [[TMP11:%.*]] = icmp slt i64 [[TMP10]], 0
; CHECK:    [[TMP12:%.*]] = icmp slt i64 0, [[TMP2]]
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP12]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP11]], i64 [[TMP13]], i64 [[TMP5]]
; CHECK:    [[TMP15:%.*]] = insertelement <4 x i64> poison, i64 [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP17:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP18:%.*]] = bitcast i64 [[TMP16]] to i64
; CHECK:    [[TMP19:%.*]] = bitcast i64 [[TMP17]] to i64
; CHECK:    [[TMP20:%.*]] = sub i64 [[TMP18]], [[TMP19]]
; CHECK:    [[TMP21:%.*]] = xor i64 [[TMP19]], -1
; CHECK:    [[TMP22:%.*]] = xor i64 [[TMP18]], [[TMP21]]
; CHECK:    [[TMP23:%.*]] = xor i64 [[TMP20]], [[TMP18]]
; CHECK:    [[TMP24:%.*]] = xor i64 [[TMP22]], -1
; CHECK:    [[TMP25:%.*]] = and i64 [[TMP24]], [[TMP23]]
; CHECK:    [[TMP26:%.*]] = icmp slt i64 [[TMP25]], 0
; CHECK:    [[TMP27:%.*]] = icmp slt i64 0, [[TMP17]]
; CHECK:    [[TMP28:%.*]] = select i1 [[TMP27]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP26]], i64 [[TMP28]], i64 [[TMP20]]
; CHECK:    [[TMP30:%.*]] = insertelement <4 x i64> [[TMP15]], i64 [[TMP29]], i64 1
; CHECK:    [[TMP31:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP32:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP33:%.*]] = bitcast i64 [[TMP31]] to i64
; CHECK:    [[TMP34:%.*]] = bitcast i64 [[TMP32]] to i64
; CHECK:    [[TMP35:%.*]] = sub i64 [[TMP33]], [[TMP34]]
; CHECK:    [[TMP36:%.*]] = xor i64 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = xor i64 [[TMP33]], [[TMP36]]
; CHECK:    [[TMP38:%.*]] = xor i64 [[TMP35]], [[TMP33]]
; CHECK:    [[TMP39:%.*]] = xor i64 [[TMP37]], -1
; CHECK:    [[TMP40:%.*]] = and i64 [[TMP39]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = icmp slt i64 [[TMP40]], 0
; CHECK:    [[TMP42:%.*]] = icmp slt i64 0, [[TMP32]]
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP42]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP44:%.*]] = select i1 [[TMP41]], i64 [[TMP43]], i64 [[TMP35]]
; CHECK:    [[TMP45:%.*]] = insertelement <4 x i64> [[TMP30]], i64 [[TMP44]], i64 2
; CHECK:    [[TMP46:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP47:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP48:%.*]] = bitcast i64 [[TMP46]] to i64
; CHECK:    [[TMP49:%.*]] = bitcast i64 [[TMP47]] to i64
; CHECK:    [[TMP50:%.*]] = sub i64 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = xor i64 [[TMP49]], -1
; CHECK:    [[TMP52:%.*]] = xor i64 [[TMP48]], [[TMP51]]
; CHECK:    [[TMP53:%.*]] = xor i64 [[TMP50]], [[TMP48]]
; CHECK:    [[TMP54:%.*]] = xor i64 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i64 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i64 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i64 0, [[TMP47]]
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i64 -9223372036854775808, i64 9223372036854775807
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i64 [[TMP58]], i64 [[TMP50]]
; CHECK:    [[TMP60:%.*]] = insertelement <4 x i64> [[TMP45]], i64 [[TMP59]], i64 3
; CHECK:    ret <4 x i64> [[TMP60]]
;
  %1 = call <4 x i64> @llvm.ssub.sat.v4i64(<4 x i64> %s1, <4 x i64> %s2)
  ret <4 x i64> %1
}

define <4 x i64> @test_sadd_vec(<4 x i64> %s1, <4 x i64> %s2) {
; CHECK-LABEL: define <4 x i64> @test_sadd_vec(
; CHECK-SAME: <4 x i64> [[S1:%.*]], <4 x i64> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i64> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i64> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP1]] to i64
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[TMP2]] to i64
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i64 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP7:%.*]] = xor i64 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP8:%.*]] = xor i64 [[TMP6]], -1
; CHECK:    [[TMP9:%.*]] = and i64 [[TMP8]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp slt i64 [[TMP9]], 0
; CHECK:    [[TMP11:%.*]] = icmp slt i64 0, [[TMP2]]
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP11]], i64 9223372036854775807, i64 -9223372036854775808
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP10]], i64 [[TMP12]], i64 [[TMP5]]
; CHECK:    [[TMP14:%.*]] = insertelement <4 x i64> poison, i64 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <4 x i64> [[S1]], i64 1
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i64> [[S2]], i64 1
; CHECK:    [[TMP17:%.*]] = bitcast i64 [[TMP15]] to i64
; CHECK:    [[TMP18:%.*]] = bitcast i64 [[TMP16]] to i64
; CHECK:    [[TMP19:%.*]] = add i64 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = xor i64 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP21:%.*]] = xor i64 [[TMP19]], [[TMP17]]
; CHECK:    [[TMP22:%.*]] = xor i64 [[TMP20]], -1
; CHECK:    [[TMP23:%.*]] = and i64 [[TMP22]], [[TMP21]]
; CHECK:    [[TMP24:%.*]] = icmp slt i64 [[TMP23]], 0
; CHECK:    [[TMP25:%.*]] = icmp slt i64 0, [[TMP16]]
; CHECK:    [[TMP26:%.*]] = select i1 [[TMP25]], i64 9223372036854775807, i64 -9223372036854775808
; CHECK:    [[TMP27:%.*]] = select i1 [[TMP24]], i64 [[TMP26]], i64 [[TMP19]]
; CHECK:    [[TMP28:%.*]] = insertelement <4 x i64> [[TMP14]], i64 [[TMP27]], i64 1
; CHECK:    [[TMP29:%.*]] = extractelement <4 x i64> [[S1]], i64 2
; CHECK:    [[TMP30:%.*]] = extractelement <4 x i64> [[S2]], i64 2
; CHECK:    [[TMP31:%.*]] = bitcast i64 [[TMP29]] to i64
; CHECK:    [[TMP32:%.*]] = bitcast i64 [[TMP30]] to i64
; CHECK:    [[TMP33:%.*]] = add i64 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = xor i64 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP35:%.*]] = xor i64 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP36:%.*]] = xor i64 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = and i64 [[TMP36]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = icmp slt i64 [[TMP37]], 0
; CHECK:    [[TMP39:%.*]] = icmp slt i64 0, [[TMP30]]
; CHECK:    [[TMP40:%.*]] = select i1 [[TMP39]], i64 9223372036854775807, i64 -9223372036854775808
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP38]], i64 [[TMP40]], i64 [[TMP33]]
; CHECK:    [[TMP42:%.*]] = insertelement <4 x i64> [[TMP28]], i64 [[TMP41]], i64 2
; CHECK:    [[TMP43:%.*]] = extractelement <4 x i64> [[S1]], i64 3
; CHECK:    [[TMP44:%.*]] = extractelement <4 x i64> [[S2]], i64 3
; CHECK:    [[TMP45:%.*]] = bitcast i64 [[TMP43]] to i64
; CHECK:    [[TMP46:%.*]] = bitcast i64 [[TMP44]] to i64
; CHECK:    [[TMP47:%.*]] = add i64 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i64 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i64 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = xor i64 [[TMP48]], -1
; CHECK:    [[TMP51:%.*]] = and i64 [[TMP50]], [[TMP49]]
; CHECK:    [[TMP52:%.*]] = icmp slt i64 [[TMP51]], 0
; CHECK:    [[TMP53:%.*]] = icmp slt i64 0, [[TMP44]]
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP53]], i64 9223372036854775807, i64 -9223372036854775808
; CHECK:    [[TMP55:%.*]] = select i1 [[TMP52]], i64 [[TMP54]], i64 [[TMP47]]
; CHECK:    [[TMP56:%.*]] = insertelement <4 x i64> [[TMP42]], i64 [[TMP55]], i64 3
; CHECK:    ret <4 x i64> [[TMP56]]
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
; CHECK:    [[TMP3:%.*]] = bitcast i32 [[TMP1]] to i32
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP2]] to i32
; CHECK:    [[TMP5:%.*]] = sub i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i32 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = xor i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = xor i32 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP9:%.*]] = xor i32 [[TMP7]], -1
; CHECK:    [[TMP10:%.*]] = and i32 [[TMP9]], [[TMP8]]
; CHECK:    [[TMP11:%.*]] = icmp slt i32 [[TMP10]], 0
; CHECK:    [[TMP12:%.*]] = icmp slt i32 0, [[TMP2]]
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP12]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP11]], i32 [[TMP13]], i32 [[TMP5]]
; CHECK:    [[TMP15:%.*]] = insertelement <4 x i32> poison, i32 [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP17:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP18:%.*]] = bitcast i32 [[TMP16]] to i32
; CHECK:    [[TMP19:%.*]] = bitcast i32 [[TMP17]] to i32
; CHECK:    [[TMP20:%.*]] = sub i32 [[TMP18]], [[TMP19]]
; CHECK:    [[TMP21:%.*]] = xor i32 [[TMP19]], -1
; CHECK:    [[TMP22:%.*]] = xor i32 [[TMP18]], [[TMP21]]
; CHECK:    [[TMP23:%.*]] = xor i32 [[TMP20]], [[TMP18]]
; CHECK:    [[TMP24:%.*]] = xor i32 [[TMP22]], -1
; CHECK:    [[TMP25:%.*]] = and i32 [[TMP24]], [[TMP23]]
; CHECK:    [[TMP26:%.*]] = icmp slt i32 [[TMP25]], 0
; CHECK:    [[TMP27:%.*]] = icmp slt i32 0, [[TMP17]]
; CHECK:    [[TMP28:%.*]] = select i1 [[TMP27]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP26]], i32 [[TMP28]], i32 [[TMP20]]
; CHECK:    [[TMP30:%.*]] = insertelement <4 x i32> [[TMP15]], i32 [[TMP29]], i64 1
; CHECK:    [[TMP31:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP32:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP33:%.*]] = bitcast i32 [[TMP31]] to i32
; CHECK:    [[TMP34:%.*]] = bitcast i32 [[TMP32]] to i32
; CHECK:    [[TMP35:%.*]] = sub i32 [[TMP33]], [[TMP34]]
; CHECK:    [[TMP36:%.*]] = xor i32 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = xor i32 [[TMP33]], [[TMP36]]
; CHECK:    [[TMP38:%.*]] = xor i32 [[TMP35]], [[TMP33]]
; CHECK:    [[TMP39:%.*]] = xor i32 [[TMP37]], -1
; CHECK:    [[TMP40:%.*]] = and i32 [[TMP39]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = icmp slt i32 [[TMP40]], 0
; CHECK:    [[TMP42:%.*]] = icmp slt i32 0, [[TMP32]]
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP42]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP44:%.*]] = select i1 [[TMP41]], i32 [[TMP43]], i32 [[TMP35]]
; CHECK:    [[TMP45:%.*]] = insertelement <4 x i32> [[TMP30]], i32 [[TMP44]], i64 2
; CHECK:    [[TMP46:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP47:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP48:%.*]] = bitcast i32 [[TMP46]] to i32
; CHECK:    [[TMP49:%.*]] = bitcast i32 [[TMP47]] to i32
; CHECK:    [[TMP50:%.*]] = sub i32 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = xor i32 [[TMP49]], -1
; CHECK:    [[TMP52:%.*]] = xor i32 [[TMP48]], [[TMP51]]
; CHECK:    [[TMP53:%.*]] = xor i32 [[TMP50]], [[TMP48]]
; CHECK:    [[TMP54:%.*]] = xor i32 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i32 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i32 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i32 0, [[TMP47]]
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i32 -2147483648, i32 2147483647
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i32 [[TMP58]], i32 [[TMP50]]
; CHECK:    [[TMP60:%.*]] = insertelement <4 x i32> [[TMP45]], i32 [[TMP59]], i64 3
; CHECK:    ret <4 x i32> [[TMP60]]
;
  %1 = call <4 x i32> @llvm.ssub.sat.v4i32(<4 x i32> %s1, <4 x i32> %s2)
  ret <4 x i32> %1
}

define <4 x i32> @test_sadd_vec_i32(<4 x i32> %s1, <4 x i32> %s2) {
; CHECK-LABEL: define <4 x i32> @test_sadd_vec_i32(
; CHECK-SAME: <4 x i32> [[S1:%.*]], <4 x i32> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = bitcast i32 [[TMP1]] to i32
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP2]] to i32
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP7:%.*]] = xor i32 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP8:%.*]] = xor i32 [[TMP6]], -1
; CHECK:    [[TMP9:%.*]] = and i32 [[TMP8]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp slt i32 [[TMP9]], 0
; CHECK:    [[TMP11:%.*]] = icmp slt i32 0, [[TMP2]]
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP11]], i32 2147483647, i32 -2147483648
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP10]], i32 [[TMP12]], i32 [[TMP5]]
; CHECK:    [[TMP14:%.*]] = insertelement <4 x i32> poison, i32 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <4 x i32> [[S1]], i64 1
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i32> [[S2]], i64 1
; CHECK:    [[TMP17:%.*]] = bitcast i32 [[TMP15]] to i32
; CHECK:    [[TMP18:%.*]] = bitcast i32 [[TMP16]] to i32
; CHECK:    [[TMP19:%.*]] = add i32 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = xor i32 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP21:%.*]] = xor i32 [[TMP19]], [[TMP17]]
; CHECK:    [[TMP22:%.*]] = xor i32 [[TMP20]], -1
; CHECK:    [[TMP23:%.*]] = and i32 [[TMP22]], [[TMP21]]
; CHECK:    [[TMP24:%.*]] = icmp slt i32 [[TMP23]], 0
; CHECK:    [[TMP25:%.*]] = icmp slt i32 0, [[TMP16]]
; CHECK:    [[TMP26:%.*]] = select i1 [[TMP25]], i32 2147483647, i32 -2147483648
; CHECK:    [[TMP27:%.*]] = select i1 [[TMP24]], i32 [[TMP26]], i32 [[TMP19]]
; CHECK:    [[TMP28:%.*]] = insertelement <4 x i32> [[TMP14]], i32 [[TMP27]], i64 1
; CHECK:    [[TMP29:%.*]] = extractelement <4 x i32> [[S1]], i64 2
; CHECK:    [[TMP30:%.*]] = extractelement <4 x i32> [[S2]], i64 2
; CHECK:    [[TMP31:%.*]] = bitcast i32 [[TMP29]] to i32
; CHECK:    [[TMP32:%.*]] = bitcast i32 [[TMP30]] to i32
; CHECK:    [[TMP33:%.*]] = add i32 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = xor i32 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP35:%.*]] = xor i32 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP36:%.*]] = xor i32 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = and i32 [[TMP36]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = icmp slt i32 [[TMP37]], 0
; CHECK:    [[TMP39:%.*]] = icmp slt i32 0, [[TMP30]]
; CHECK:    [[TMP40:%.*]] = select i1 [[TMP39]], i32 2147483647, i32 -2147483648
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP38]], i32 [[TMP40]], i32 [[TMP33]]
; CHECK:    [[TMP42:%.*]] = insertelement <4 x i32> [[TMP28]], i32 [[TMP41]], i64 2
; CHECK:    [[TMP43:%.*]] = extractelement <4 x i32> [[S1]], i64 3
; CHECK:    [[TMP44:%.*]] = extractelement <4 x i32> [[S2]], i64 3
; CHECK:    [[TMP45:%.*]] = bitcast i32 [[TMP43]] to i32
; CHECK:    [[TMP46:%.*]] = bitcast i32 [[TMP44]] to i32
; CHECK:    [[TMP47:%.*]] = add i32 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i32 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i32 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = xor i32 [[TMP48]], -1
; CHECK:    [[TMP51:%.*]] = and i32 [[TMP50]], [[TMP49]]
; CHECK:    [[TMP52:%.*]] = icmp slt i32 [[TMP51]], 0
; CHECK:    [[TMP53:%.*]] = icmp slt i32 0, [[TMP44]]
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP53]], i32 2147483647, i32 -2147483648
; CHECK:    [[TMP55:%.*]] = select i1 [[TMP52]], i32 [[TMP54]], i32 [[TMP47]]
; CHECK:    [[TMP56:%.*]] = insertelement <4 x i32> [[TMP42]], i32 [[TMP55]], i64 3
; CHECK:    ret <4 x i32> [[TMP56]]
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
; CHECK:    [[TMP3:%.*]] = bitcast i16 [[TMP1]] to i16
; CHECK:    [[TMP4:%.*]] = bitcast i16 [[TMP2]] to i16
; CHECK:    [[TMP5:%.*]] = sub i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i16 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = xor i16 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = xor i16 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP9:%.*]] = xor i16 [[TMP7]], -1
; CHECK:    [[TMP10:%.*]] = and i16 [[TMP9]], [[TMP8]]
; CHECK:    [[TMP11:%.*]] = icmp slt i16 [[TMP10]], 0
; CHECK:    [[TMP12:%.*]] = icmp slt i16 0, [[TMP2]]
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP12]], i16 -32768, i16 32767
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP11]], i16 [[TMP13]], i16 [[TMP5]]
; CHECK:    [[TMP15:%.*]] = insertelement <8 x i16> poison, i16 [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP17:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP18:%.*]] = bitcast i16 [[TMP16]] to i16
; CHECK:    [[TMP19:%.*]] = bitcast i16 [[TMP17]] to i16
; CHECK:    [[TMP20:%.*]] = sub i16 [[TMP18]], [[TMP19]]
; CHECK:    [[TMP21:%.*]] = xor i16 [[TMP19]], -1
; CHECK:    [[TMP22:%.*]] = xor i16 [[TMP18]], [[TMP21]]
; CHECK:    [[TMP23:%.*]] = xor i16 [[TMP20]], [[TMP18]]
; CHECK:    [[TMP24:%.*]] = xor i16 [[TMP22]], -1
; CHECK:    [[TMP25:%.*]] = and i16 [[TMP24]], [[TMP23]]
; CHECK:    [[TMP26:%.*]] = icmp slt i16 [[TMP25]], 0
; CHECK:    [[TMP27:%.*]] = icmp slt i16 0, [[TMP17]]
; CHECK:    [[TMP28:%.*]] = select i1 [[TMP27]], i16 -32768, i16 32767
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP26]], i16 [[TMP28]], i16 [[TMP20]]
; CHECK:    [[TMP30:%.*]] = insertelement <8 x i16> [[TMP15]], i16 [[TMP29]], i64 1
; CHECK:    [[TMP31:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP32:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP33:%.*]] = bitcast i16 [[TMP31]] to i16
; CHECK:    [[TMP34:%.*]] = bitcast i16 [[TMP32]] to i16
; CHECK:    [[TMP35:%.*]] = sub i16 [[TMP33]], [[TMP34]]
; CHECK:    [[TMP36:%.*]] = xor i16 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = xor i16 [[TMP33]], [[TMP36]]
; CHECK:    [[TMP38:%.*]] = xor i16 [[TMP35]], [[TMP33]]
; CHECK:    [[TMP39:%.*]] = xor i16 [[TMP37]], -1
; CHECK:    [[TMP40:%.*]] = and i16 [[TMP39]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = icmp slt i16 [[TMP40]], 0
; CHECK:    [[TMP42:%.*]] = icmp slt i16 0, [[TMP32]]
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP42]], i16 -32768, i16 32767
; CHECK:    [[TMP44:%.*]] = select i1 [[TMP41]], i16 [[TMP43]], i16 [[TMP35]]
; CHECK:    [[TMP45:%.*]] = insertelement <8 x i16> [[TMP30]], i16 [[TMP44]], i64 2
; CHECK:    [[TMP46:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP47:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP48:%.*]] = bitcast i16 [[TMP46]] to i16
; CHECK:    [[TMP49:%.*]] = bitcast i16 [[TMP47]] to i16
; CHECK:    [[TMP50:%.*]] = sub i16 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = xor i16 [[TMP49]], -1
; CHECK:    [[TMP52:%.*]] = xor i16 [[TMP48]], [[TMP51]]
; CHECK:    [[TMP53:%.*]] = xor i16 [[TMP50]], [[TMP48]]
; CHECK:    [[TMP54:%.*]] = xor i16 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i16 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i16 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i16 0, [[TMP47]]
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i16 -32768, i16 32767
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i16 [[TMP58]], i16 [[TMP50]]
; CHECK:    [[TMP60:%.*]] = insertelement <8 x i16> [[TMP45]], i16 [[TMP59]], i64 3
; CHECK:    [[TMP61:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP62:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP63:%.*]] = bitcast i16 [[TMP61]] to i16
; CHECK:    [[TMP64:%.*]] = bitcast i16 [[TMP62]] to i16
; CHECK:    [[TMP65:%.*]] = sub i16 [[TMP63]], [[TMP64]]
; CHECK:    [[TMP66:%.*]] = xor i16 [[TMP64]], -1
; CHECK:    [[TMP67:%.*]] = xor i16 [[TMP63]], [[TMP66]]
; CHECK:    [[TMP68:%.*]] = xor i16 [[TMP65]], [[TMP63]]
; CHECK:    [[TMP69:%.*]] = xor i16 [[TMP67]], -1
; CHECK:    [[TMP70:%.*]] = and i16 [[TMP69]], [[TMP68]]
; CHECK:    [[TMP71:%.*]] = icmp slt i16 [[TMP70]], 0
; CHECK:    [[TMP72:%.*]] = icmp slt i16 0, [[TMP62]]
; CHECK:    [[TMP73:%.*]] = select i1 [[TMP72]], i16 -32768, i16 32767
; CHECK:    [[TMP74:%.*]] = select i1 [[TMP71]], i16 [[TMP73]], i16 [[TMP65]]
; CHECK:    [[TMP75:%.*]] = insertelement <8 x i16> [[TMP60]], i16 [[TMP74]], i64 4
; CHECK:    [[TMP76:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP77:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP78:%.*]] = bitcast i16 [[TMP76]] to i16
; CHECK:    [[TMP79:%.*]] = bitcast i16 [[TMP77]] to i16
; CHECK:    [[TMP80:%.*]] = sub i16 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP81:%.*]] = xor i16 [[TMP79]], -1
; CHECK:    [[TMP82:%.*]] = xor i16 [[TMP78]], [[TMP81]]
; CHECK:    [[TMP83:%.*]] = xor i16 [[TMP80]], [[TMP78]]
; CHECK:    [[TMP84:%.*]] = xor i16 [[TMP82]], -1
; CHECK:    [[TMP85:%.*]] = and i16 [[TMP84]], [[TMP83]]
; CHECK:    [[TMP86:%.*]] = icmp slt i16 [[TMP85]], 0
; CHECK:    [[TMP87:%.*]] = icmp slt i16 0, [[TMP77]]
; CHECK:    [[TMP88:%.*]] = select i1 [[TMP87]], i16 -32768, i16 32767
; CHECK:    [[TMP89:%.*]] = select i1 [[TMP86]], i16 [[TMP88]], i16 [[TMP80]]
; CHECK:    [[TMP90:%.*]] = insertelement <8 x i16> [[TMP75]], i16 [[TMP89]], i64 5
; CHECK:    [[TMP91:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP92:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP93:%.*]] = bitcast i16 [[TMP91]] to i16
; CHECK:    [[TMP94:%.*]] = bitcast i16 [[TMP92]] to i16
; CHECK:    [[TMP95:%.*]] = sub i16 [[TMP93]], [[TMP94]]
; CHECK:    [[TMP96:%.*]] = xor i16 [[TMP94]], -1
; CHECK:    [[TMP97:%.*]] = xor i16 [[TMP93]], [[TMP96]]
; CHECK:    [[TMP98:%.*]] = xor i16 [[TMP95]], [[TMP93]]
; CHECK:    [[TMP99:%.*]] = xor i16 [[TMP97]], -1
; CHECK:    [[TMP100:%.*]] = and i16 [[TMP99]], [[TMP98]]
; CHECK:    [[TMP101:%.*]] = icmp slt i16 [[TMP100]], 0
; CHECK:    [[TMP102:%.*]] = icmp slt i16 0, [[TMP92]]
; CHECK:    [[TMP103:%.*]] = select i1 [[TMP102]], i16 -32768, i16 32767
; CHECK:    [[TMP104:%.*]] = select i1 [[TMP101]], i16 [[TMP103]], i16 [[TMP95]]
; CHECK:    [[TMP105:%.*]] = insertelement <8 x i16> [[TMP90]], i16 [[TMP104]], i64 6
; CHECK:    [[TMP106:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP107:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP108:%.*]] = bitcast i16 [[TMP106]] to i16
; CHECK:    [[TMP109:%.*]] = bitcast i16 [[TMP107]] to i16
; CHECK:    [[TMP110:%.*]] = sub i16 [[TMP108]], [[TMP109]]
; CHECK:    [[TMP111:%.*]] = xor i16 [[TMP109]], -1
; CHECK:    [[TMP112:%.*]] = xor i16 [[TMP108]], [[TMP111]]
; CHECK:    [[TMP113:%.*]] = xor i16 [[TMP110]], [[TMP108]]
; CHECK:    [[TMP114:%.*]] = xor i16 [[TMP112]], -1
; CHECK:    [[TMP115:%.*]] = and i16 [[TMP114]], [[TMP113]]
; CHECK:    [[TMP116:%.*]] = icmp slt i16 [[TMP115]], 0
; CHECK:    [[TMP117:%.*]] = icmp slt i16 0, [[TMP107]]
; CHECK:    [[TMP118:%.*]] = select i1 [[TMP117]], i16 -32768, i16 32767
; CHECK:    [[TMP119:%.*]] = select i1 [[TMP116]], i16 [[TMP118]], i16 [[TMP110]]
; CHECK:    [[TMP120:%.*]] = insertelement <8 x i16> [[TMP105]], i16 [[TMP119]], i64 7
; CHECK:    ret <8 x i16> [[TMP120]]
;
  %1 = call <8 x i16> @llvm.ssub.sat.v8i16(<8 x i16> %s1, <8 x i16> %s2)
  ret <8 x i16> %1
}

define <8 x i16> @test_sadd_vec_i16(<8 x i16> %s1, <8 x i16> %s2) {
; CHECK-LABEL: define <8 x i16> @test_sadd_vec_i16(
; CHECK-SAME: <8 x i16> [[S1:%.*]], <8 x i16> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <8 x i16> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <8 x i16> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = bitcast i16 [[TMP1]] to i16
; CHECK:    [[TMP4:%.*]] = bitcast i16 [[TMP2]] to i16
; CHECK:    [[TMP5:%.*]] = add i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP7:%.*]] = xor i16 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP8:%.*]] = xor i16 [[TMP6]], -1
; CHECK:    [[TMP9:%.*]] = and i16 [[TMP8]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp slt i16 [[TMP9]], 0
; CHECK:    [[TMP11:%.*]] = icmp slt i16 0, [[TMP2]]
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP11]], i16 32767, i16 -32768
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP10]], i16 [[TMP12]], i16 [[TMP5]]
; CHECK:    [[TMP14:%.*]] = insertelement <8 x i16> poison, i16 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <8 x i16> [[S1]], i64 1
; CHECK:    [[TMP16:%.*]] = extractelement <8 x i16> [[S2]], i64 1
; CHECK:    [[TMP17:%.*]] = bitcast i16 [[TMP15]] to i16
; CHECK:    [[TMP18:%.*]] = bitcast i16 [[TMP16]] to i16
; CHECK:    [[TMP19:%.*]] = add i16 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = xor i16 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP21:%.*]] = xor i16 [[TMP19]], [[TMP17]]
; CHECK:    [[TMP22:%.*]] = xor i16 [[TMP20]], -1
; CHECK:    [[TMP23:%.*]] = and i16 [[TMP22]], [[TMP21]]
; CHECK:    [[TMP24:%.*]] = icmp slt i16 [[TMP23]], 0
; CHECK:    [[TMP25:%.*]] = icmp slt i16 0, [[TMP16]]
; CHECK:    [[TMP26:%.*]] = select i1 [[TMP25]], i16 32767, i16 -32768
; CHECK:    [[TMP27:%.*]] = select i1 [[TMP24]], i16 [[TMP26]], i16 [[TMP19]]
; CHECK:    [[TMP28:%.*]] = insertelement <8 x i16> [[TMP14]], i16 [[TMP27]], i64 1
; CHECK:    [[TMP29:%.*]] = extractelement <8 x i16> [[S1]], i64 2
; CHECK:    [[TMP30:%.*]] = extractelement <8 x i16> [[S2]], i64 2
; CHECK:    [[TMP31:%.*]] = bitcast i16 [[TMP29]] to i16
; CHECK:    [[TMP32:%.*]] = bitcast i16 [[TMP30]] to i16
; CHECK:    [[TMP33:%.*]] = add i16 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = xor i16 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP35:%.*]] = xor i16 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP36:%.*]] = xor i16 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = and i16 [[TMP36]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = icmp slt i16 [[TMP37]], 0
; CHECK:    [[TMP39:%.*]] = icmp slt i16 0, [[TMP30]]
; CHECK:    [[TMP40:%.*]] = select i1 [[TMP39]], i16 32767, i16 -32768
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP38]], i16 [[TMP40]], i16 [[TMP33]]
; CHECK:    [[TMP42:%.*]] = insertelement <8 x i16> [[TMP28]], i16 [[TMP41]], i64 2
; CHECK:    [[TMP43:%.*]] = extractelement <8 x i16> [[S1]], i64 3
; CHECK:    [[TMP44:%.*]] = extractelement <8 x i16> [[S2]], i64 3
; CHECK:    [[TMP45:%.*]] = bitcast i16 [[TMP43]] to i16
; CHECK:    [[TMP46:%.*]] = bitcast i16 [[TMP44]] to i16
; CHECK:    [[TMP47:%.*]] = add i16 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i16 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i16 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = xor i16 [[TMP48]], -1
; CHECK:    [[TMP51:%.*]] = and i16 [[TMP50]], [[TMP49]]
; CHECK:    [[TMP52:%.*]] = icmp slt i16 [[TMP51]], 0
; CHECK:    [[TMP53:%.*]] = icmp slt i16 0, [[TMP44]]
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP53]], i16 32767, i16 -32768
; CHECK:    [[TMP55:%.*]] = select i1 [[TMP52]], i16 [[TMP54]], i16 [[TMP47]]
; CHECK:    [[TMP56:%.*]] = insertelement <8 x i16> [[TMP42]], i16 [[TMP55]], i64 3
; CHECK:    [[TMP57:%.*]] = extractelement <8 x i16> [[S1]], i64 4
; CHECK:    [[TMP58:%.*]] = extractelement <8 x i16> [[S2]], i64 4
; CHECK:    [[TMP59:%.*]] = bitcast i16 [[TMP57]] to i16
; CHECK:    [[TMP60:%.*]] = bitcast i16 [[TMP58]] to i16
; CHECK:    [[TMP61:%.*]] = add i16 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP62:%.*]] = xor i16 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP63:%.*]] = xor i16 [[TMP61]], [[TMP59]]
; CHECK:    [[TMP64:%.*]] = xor i16 [[TMP62]], -1
; CHECK:    [[TMP65:%.*]] = and i16 [[TMP64]], [[TMP63]]
; CHECK:    [[TMP66:%.*]] = icmp slt i16 [[TMP65]], 0
; CHECK:    [[TMP67:%.*]] = icmp slt i16 0, [[TMP58]]
; CHECK:    [[TMP68:%.*]] = select i1 [[TMP67]], i16 32767, i16 -32768
; CHECK:    [[TMP69:%.*]] = select i1 [[TMP66]], i16 [[TMP68]], i16 [[TMP61]]
; CHECK:    [[TMP70:%.*]] = insertelement <8 x i16> [[TMP56]], i16 [[TMP69]], i64 4
; CHECK:    [[TMP71:%.*]] = extractelement <8 x i16> [[S1]], i64 5
; CHECK:    [[TMP72:%.*]] = extractelement <8 x i16> [[S2]], i64 5
; CHECK:    [[TMP73:%.*]] = bitcast i16 [[TMP71]] to i16
; CHECK:    [[TMP74:%.*]] = bitcast i16 [[TMP72]] to i16
; CHECK:    [[TMP75:%.*]] = add i16 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = xor i16 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP77:%.*]] = xor i16 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP78:%.*]] = xor i16 [[TMP76]], -1
; CHECK:    [[TMP79:%.*]] = and i16 [[TMP78]], [[TMP77]]
; CHECK:    [[TMP80:%.*]] = icmp slt i16 [[TMP79]], 0
; CHECK:    [[TMP81:%.*]] = icmp slt i16 0, [[TMP72]]
; CHECK:    [[TMP82:%.*]] = select i1 [[TMP81]], i16 32767, i16 -32768
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP80]], i16 [[TMP82]], i16 [[TMP75]]
; CHECK:    [[TMP84:%.*]] = insertelement <8 x i16> [[TMP70]], i16 [[TMP83]], i64 5
; CHECK:    [[TMP85:%.*]] = extractelement <8 x i16> [[S1]], i64 6
; CHECK:    [[TMP86:%.*]] = extractelement <8 x i16> [[S2]], i64 6
; CHECK:    [[TMP87:%.*]] = bitcast i16 [[TMP85]] to i16
; CHECK:    [[TMP88:%.*]] = bitcast i16 [[TMP86]] to i16
; CHECK:    [[TMP89:%.*]] = add i16 [[TMP87]], [[TMP88]]
; CHECK:    [[TMP90:%.*]] = xor i16 [[TMP87]], [[TMP88]]
; CHECK:    [[TMP91:%.*]] = xor i16 [[TMP89]], [[TMP87]]
; CHECK:    [[TMP92:%.*]] = xor i16 [[TMP90]], -1
; CHECK:    [[TMP93:%.*]] = and i16 [[TMP92]], [[TMP91]]
; CHECK:    [[TMP94:%.*]] = icmp slt i16 [[TMP93]], 0
; CHECK:    [[TMP95:%.*]] = icmp slt i16 0, [[TMP86]]
; CHECK:    [[TMP96:%.*]] = select i1 [[TMP95]], i16 32767, i16 -32768
; CHECK:    [[TMP97:%.*]] = select i1 [[TMP94]], i16 [[TMP96]], i16 [[TMP89]]
; CHECK:    [[TMP98:%.*]] = insertelement <8 x i16> [[TMP84]], i16 [[TMP97]], i64 6
; CHECK:    [[TMP99:%.*]] = extractelement <8 x i16> [[S1]], i64 7
; CHECK:    [[TMP100:%.*]] = extractelement <8 x i16> [[S2]], i64 7
; CHECK:    [[TMP101:%.*]] = bitcast i16 [[TMP99]] to i16
; CHECK:    [[TMP102:%.*]] = bitcast i16 [[TMP100]] to i16
; CHECK:    [[TMP103:%.*]] = add i16 [[TMP101]], [[TMP102]]
; CHECK:    [[TMP104:%.*]] = xor i16 [[TMP101]], [[TMP102]]
; CHECK:    [[TMP105:%.*]] = xor i16 [[TMP103]], [[TMP101]]
; CHECK:    [[TMP106:%.*]] = xor i16 [[TMP104]], -1
; CHECK:    [[TMP107:%.*]] = and i16 [[TMP106]], [[TMP105]]
; CHECK:    [[TMP108:%.*]] = icmp slt i16 [[TMP107]], 0
; CHECK:    [[TMP109:%.*]] = icmp slt i16 0, [[TMP100]]
; CHECK:    [[TMP110:%.*]] = select i1 [[TMP109]], i16 32767, i16 -32768
; CHECK:    [[TMP111:%.*]] = select i1 [[TMP108]], i16 [[TMP110]], i16 [[TMP103]]
; CHECK:    [[TMP112:%.*]] = insertelement <8 x i16> [[TMP98]], i16 [[TMP111]], i64 7
; CHECK:    ret <8 x i16> [[TMP112]]
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
; CHECK:    [[TMP3:%.*]] = bitcast i8 [[TMP1]] to i8
; CHECK:    [[TMP4:%.*]] = bitcast i8 [[TMP2]] to i8
; CHECK:    [[TMP5:%.*]] = add i8 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i8 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP7:%.*]] = xor i8 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP8:%.*]] = xor i8 [[TMP6]], -1
; CHECK:    [[TMP9:%.*]] = and i8 [[TMP8]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp slt i8 [[TMP9]], 0
; CHECK:    [[TMP11:%.*]] = icmp slt i8 0, [[TMP2]]
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP11]], i8 127, i8 -128
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP10]], i8 [[TMP12]], i8 [[TMP5]]
; CHECK:    [[TMP14:%.*]] = insertelement <16 x i8> poison, i8 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP16:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP17:%.*]] = bitcast i8 [[TMP15]] to i8
; CHECK:    [[TMP18:%.*]] = bitcast i8 [[TMP16]] to i8
; CHECK:    [[TMP19:%.*]] = add i8 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = xor i8 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP21:%.*]] = xor i8 [[TMP19]], [[TMP17]]
; CHECK:    [[TMP22:%.*]] = xor i8 [[TMP20]], -1
; CHECK:    [[TMP23:%.*]] = and i8 [[TMP22]], [[TMP21]]
; CHECK:    [[TMP24:%.*]] = icmp slt i8 [[TMP23]], 0
; CHECK:    [[TMP25:%.*]] = icmp slt i8 0, [[TMP16]]
; CHECK:    [[TMP26:%.*]] = select i1 [[TMP25]], i8 127, i8 -128
; CHECK:    [[TMP27:%.*]] = select i1 [[TMP24]], i8 [[TMP26]], i8 [[TMP19]]
; CHECK:    [[TMP28:%.*]] = insertelement <16 x i8> [[TMP14]], i8 [[TMP27]], i64 1
; CHECK:    [[TMP29:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP30:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP31:%.*]] = bitcast i8 [[TMP29]] to i8
; CHECK:    [[TMP32:%.*]] = bitcast i8 [[TMP30]] to i8
; CHECK:    [[TMP33:%.*]] = add i8 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP34:%.*]] = xor i8 [[TMP31]], [[TMP32]]
; CHECK:    [[TMP35:%.*]] = xor i8 [[TMP33]], [[TMP31]]
; CHECK:    [[TMP36:%.*]] = xor i8 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = and i8 [[TMP36]], [[TMP35]]
; CHECK:    [[TMP38:%.*]] = icmp slt i8 [[TMP37]], 0
; CHECK:    [[TMP39:%.*]] = icmp slt i8 0, [[TMP30]]
; CHECK:    [[TMP40:%.*]] = select i1 [[TMP39]], i8 127, i8 -128
; CHECK:    [[TMP41:%.*]] = select i1 [[TMP38]], i8 [[TMP40]], i8 [[TMP33]]
; CHECK:    [[TMP42:%.*]] = insertelement <16 x i8> [[TMP28]], i8 [[TMP41]], i64 2
; CHECK:    [[TMP43:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP44:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP45:%.*]] = bitcast i8 [[TMP43]] to i8
; CHECK:    [[TMP46:%.*]] = bitcast i8 [[TMP44]] to i8
; CHECK:    [[TMP47:%.*]] = add i8 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP48:%.*]] = xor i8 [[TMP45]], [[TMP46]]
; CHECK:    [[TMP49:%.*]] = xor i8 [[TMP47]], [[TMP45]]
; CHECK:    [[TMP50:%.*]] = xor i8 [[TMP48]], -1
; CHECK:    [[TMP51:%.*]] = and i8 [[TMP50]], [[TMP49]]
; CHECK:    [[TMP52:%.*]] = icmp slt i8 [[TMP51]], 0
; CHECK:    [[TMP53:%.*]] = icmp slt i8 0, [[TMP44]]
; CHECK:    [[TMP54:%.*]] = select i1 [[TMP53]], i8 127, i8 -128
; CHECK:    [[TMP55:%.*]] = select i1 [[TMP52]], i8 [[TMP54]], i8 [[TMP47]]
; CHECK:    [[TMP56:%.*]] = insertelement <16 x i8> [[TMP42]], i8 [[TMP55]], i64 3
; CHECK:    [[TMP57:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP58:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP59:%.*]] = bitcast i8 [[TMP57]] to i8
; CHECK:    [[TMP60:%.*]] = bitcast i8 [[TMP58]] to i8
; CHECK:    [[TMP61:%.*]] = add i8 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP62:%.*]] = xor i8 [[TMP59]], [[TMP60]]
; CHECK:    [[TMP63:%.*]] = xor i8 [[TMP61]], [[TMP59]]
; CHECK:    [[TMP64:%.*]] = xor i8 [[TMP62]], -1
; CHECK:    [[TMP65:%.*]] = and i8 [[TMP64]], [[TMP63]]
; CHECK:    [[TMP66:%.*]] = icmp slt i8 [[TMP65]], 0
; CHECK:    [[TMP67:%.*]] = icmp slt i8 0, [[TMP58]]
; CHECK:    [[TMP68:%.*]] = select i1 [[TMP67]], i8 127, i8 -128
; CHECK:    [[TMP69:%.*]] = select i1 [[TMP66]], i8 [[TMP68]], i8 [[TMP61]]
; CHECK:    [[TMP70:%.*]] = insertelement <16 x i8> [[TMP56]], i8 [[TMP69]], i64 4
; CHECK:    [[TMP71:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP72:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP73:%.*]] = bitcast i8 [[TMP71]] to i8
; CHECK:    [[TMP74:%.*]] = bitcast i8 [[TMP72]] to i8
; CHECK:    [[TMP75:%.*]] = add i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP76:%.*]] = xor i8 [[TMP73]], [[TMP74]]
; CHECK:    [[TMP77:%.*]] = xor i8 [[TMP75]], [[TMP73]]
; CHECK:    [[TMP78:%.*]] = xor i8 [[TMP76]], -1
; CHECK:    [[TMP79:%.*]] = and i8 [[TMP78]], [[TMP77]]
; CHECK:    [[TMP80:%.*]] = icmp slt i8 [[TMP79]], 0
; CHECK:    [[TMP81:%.*]] = icmp slt i8 0, [[TMP72]]
; CHECK:    [[TMP82:%.*]] = select i1 [[TMP81]], i8 127, i8 -128
; CHECK:    [[TMP83:%.*]] = select i1 [[TMP80]], i8 [[TMP82]], i8 [[TMP75]]
; CHECK:    [[TMP84:%.*]] = insertelement <16 x i8> [[TMP70]], i8 [[TMP83]], i64 5
; CHECK:    [[TMP85:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP86:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP87:%.*]] = bitcast i8 [[TMP85]] to i8
; CHECK:    [[TMP88:%.*]] = bitcast i8 [[TMP86]] to i8
; CHECK:    [[TMP89:%.*]] = add i8 [[TMP87]], [[TMP88]]
; CHECK:    [[TMP90:%.*]] = xor i8 [[TMP87]], [[TMP88]]
; CHECK:    [[TMP91:%.*]] = xor i8 [[TMP89]], [[TMP87]]
; CHECK:    [[TMP92:%.*]] = xor i8 [[TMP90]], -1
; CHECK:    [[TMP93:%.*]] = and i8 [[TMP92]], [[TMP91]]
; CHECK:    [[TMP94:%.*]] = icmp slt i8 [[TMP93]], 0
; CHECK:    [[TMP95:%.*]] = icmp slt i8 0, [[TMP86]]
; CHECK:    [[TMP96:%.*]] = select i1 [[TMP95]], i8 127, i8 -128
; CHECK:    [[TMP97:%.*]] = select i1 [[TMP94]], i8 [[TMP96]], i8 [[TMP89]]
; CHECK:    [[TMP98:%.*]] = insertelement <16 x i8> [[TMP84]], i8 [[TMP97]], i64 6
; CHECK:    [[TMP99:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP100:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP101:%.*]] = bitcast i8 [[TMP99]] to i8
; CHECK:    [[TMP102:%.*]] = bitcast i8 [[TMP100]] to i8
; CHECK:    [[TMP103:%.*]] = add i8 [[TMP101]], [[TMP102]]
; CHECK:    [[TMP104:%.*]] = xor i8 [[TMP101]], [[TMP102]]
; CHECK:    [[TMP105:%.*]] = xor i8 [[TMP103]], [[TMP101]]
; CHECK:    [[TMP106:%.*]] = xor i8 [[TMP104]], -1
; CHECK:    [[TMP107:%.*]] = and i8 [[TMP106]], [[TMP105]]
; CHECK:    [[TMP108:%.*]] = icmp slt i8 [[TMP107]], 0
; CHECK:    [[TMP109:%.*]] = icmp slt i8 0, [[TMP100]]
; CHECK:    [[TMP110:%.*]] = select i1 [[TMP109]], i8 127, i8 -128
; CHECK:    [[TMP111:%.*]] = select i1 [[TMP108]], i8 [[TMP110]], i8 [[TMP103]]
; CHECK:    [[TMP112:%.*]] = insertelement <16 x i8> [[TMP98]], i8 [[TMP111]], i64 7
; CHECK:    [[TMP113:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP114:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP115:%.*]] = bitcast i8 [[TMP113]] to i8
; CHECK:    [[TMP116:%.*]] = bitcast i8 [[TMP114]] to i8
; CHECK:    [[TMP117:%.*]] = add i8 [[TMP115]], [[TMP116]]
; CHECK:    [[TMP118:%.*]] = xor i8 [[TMP115]], [[TMP116]]
; CHECK:    [[TMP119:%.*]] = xor i8 [[TMP117]], [[TMP115]]
; CHECK:    [[TMP120:%.*]] = xor i8 [[TMP118]], -1
; CHECK:    [[TMP121:%.*]] = and i8 [[TMP120]], [[TMP119]]
; CHECK:    [[TMP122:%.*]] = icmp slt i8 [[TMP121]], 0
; CHECK:    [[TMP123:%.*]] = icmp slt i8 0, [[TMP114]]
; CHECK:    [[TMP124:%.*]] = select i1 [[TMP123]], i8 127, i8 -128
; CHECK:    [[TMP125:%.*]] = select i1 [[TMP122]], i8 [[TMP124]], i8 [[TMP117]]
; CHECK:    [[TMP126:%.*]] = insertelement <16 x i8> [[TMP112]], i8 [[TMP125]], i64 8
; CHECK:    [[TMP127:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP128:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP129:%.*]] = bitcast i8 [[TMP127]] to i8
; CHECK:    [[TMP130:%.*]] = bitcast i8 [[TMP128]] to i8
; CHECK:    [[TMP131:%.*]] = add i8 [[TMP129]], [[TMP130]]
; CHECK:    [[TMP132:%.*]] = xor i8 [[TMP129]], [[TMP130]]
; CHECK:    [[TMP133:%.*]] = xor i8 [[TMP131]], [[TMP129]]
; CHECK:    [[TMP134:%.*]] = xor i8 [[TMP132]], -1
; CHECK:    [[TMP135:%.*]] = and i8 [[TMP134]], [[TMP133]]
; CHECK:    [[TMP136:%.*]] = icmp slt i8 [[TMP135]], 0
; CHECK:    [[TMP137:%.*]] = icmp slt i8 0, [[TMP128]]
; CHECK:    [[TMP138:%.*]] = select i1 [[TMP137]], i8 127, i8 -128
; CHECK:    [[TMP139:%.*]] = select i1 [[TMP136]], i8 [[TMP138]], i8 [[TMP131]]
; CHECK:    [[TMP140:%.*]] = insertelement <16 x i8> [[TMP126]], i8 [[TMP139]], i64 9
; CHECK:    [[TMP141:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP142:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP143:%.*]] = bitcast i8 [[TMP141]] to i8
; CHECK:    [[TMP144:%.*]] = bitcast i8 [[TMP142]] to i8
; CHECK:    [[TMP145:%.*]] = add i8 [[TMP143]], [[TMP144]]
; CHECK:    [[TMP146:%.*]] = xor i8 [[TMP143]], [[TMP144]]
; CHECK:    [[TMP147:%.*]] = xor i8 [[TMP145]], [[TMP143]]
; CHECK:    [[TMP148:%.*]] = xor i8 [[TMP146]], -1
; CHECK:    [[TMP149:%.*]] = and i8 [[TMP148]], [[TMP147]]
; CHECK:    [[TMP150:%.*]] = icmp slt i8 [[TMP149]], 0
; CHECK:    [[TMP151:%.*]] = icmp slt i8 0, [[TMP142]]
; CHECK:    [[TMP152:%.*]] = select i1 [[TMP151]], i8 127, i8 -128
; CHECK:    [[TMP153:%.*]] = select i1 [[TMP150]], i8 [[TMP152]], i8 [[TMP145]]
; CHECK:    [[TMP154:%.*]] = insertelement <16 x i8> [[TMP140]], i8 [[TMP153]], i64 10
; CHECK:    [[TMP155:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP156:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP157:%.*]] = bitcast i8 [[TMP155]] to i8
; CHECK:    [[TMP158:%.*]] = bitcast i8 [[TMP156]] to i8
; CHECK:    [[TMP159:%.*]] = add i8 [[TMP157]], [[TMP158]]
; CHECK:    [[TMP160:%.*]] = xor i8 [[TMP157]], [[TMP158]]
; CHECK:    [[TMP161:%.*]] = xor i8 [[TMP159]], [[TMP157]]
; CHECK:    [[TMP162:%.*]] = xor i8 [[TMP160]], -1
; CHECK:    [[TMP163:%.*]] = and i8 [[TMP162]], [[TMP161]]
; CHECK:    [[TMP164:%.*]] = icmp slt i8 [[TMP163]], 0
; CHECK:    [[TMP165:%.*]] = icmp slt i8 0, [[TMP156]]
; CHECK:    [[TMP166:%.*]] = select i1 [[TMP165]], i8 127, i8 -128
; CHECK:    [[TMP167:%.*]] = select i1 [[TMP164]], i8 [[TMP166]], i8 [[TMP159]]
; CHECK:    [[TMP168:%.*]] = insertelement <16 x i8> [[TMP154]], i8 [[TMP167]], i64 11
; CHECK:    [[TMP169:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP170:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP171:%.*]] = bitcast i8 [[TMP169]] to i8
; CHECK:    [[TMP172:%.*]] = bitcast i8 [[TMP170]] to i8
; CHECK:    [[TMP173:%.*]] = add i8 [[TMP171]], [[TMP172]]
; CHECK:    [[TMP174:%.*]] = xor i8 [[TMP171]], [[TMP172]]
; CHECK:    [[TMP175:%.*]] = xor i8 [[TMP173]], [[TMP171]]
; CHECK:    [[TMP176:%.*]] = xor i8 [[TMP174]], -1
; CHECK:    [[TMP177:%.*]] = and i8 [[TMP176]], [[TMP175]]
; CHECK:    [[TMP178:%.*]] = icmp slt i8 [[TMP177]], 0
; CHECK:    [[TMP179:%.*]] = icmp slt i8 0, [[TMP170]]
; CHECK:    [[TMP180:%.*]] = select i1 [[TMP179]], i8 127, i8 -128
; CHECK:    [[TMP181:%.*]] = select i1 [[TMP178]], i8 [[TMP180]], i8 [[TMP173]]
; CHECK:    [[TMP182:%.*]] = insertelement <16 x i8> [[TMP168]], i8 [[TMP181]], i64 12
; CHECK:    [[TMP183:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP184:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP185:%.*]] = bitcast i8 [[TMP183]] to i8
; CHECK:    [[TMP186:%.*]] = bitcast i8 [[TMP184]] to i8
; CHECK:    [[TMP187:%.*]] = add i8 [[TMP185]], [[TMP186]]
; CHECK:    [[TMP188:%.*]] = xor i8 [[TMP185]], [[TMP186]]
; CHECK:    [[TMP189:%.*]] = xor i8 [[TMP187]], [[TMP185]]
; CHECK:    [[TMP190:%.*]] = xor i8 [[TMP188]], -1
; CHECK:    [[TMP191:%.*]] = and i8 [[TMP190]], [[TMP189]]
; CHECK:    [[TMP192:%.*]] = icmp slt i8 [[TMP191]], 0
; CHECK:    [[TMP193:%.*]] = icmp slt i8 0, [[TMP184]]
; CHECK:    [[TMP194:%.*]] = select i1 [[TMP193]], i8 127, i8 -128
; CHECK:    [[TMP195:%.*]] = select i1 [[TMP192]], i8 [[TMP194]], i8 [[TMP187]]
; CHECK:    [[TMP196:%.*]] = insertelement <16 x i8> [[TMP182]], i8 [[TMP195]], i64 13
; CHECK:    [[TMP197:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP198:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP199:%.*]] = bitcast i8 [[TMP197]] to i8
; CHECK:    [[TMP200:%.*]] = bitcast i8 [[TMP198]] to i8
; CHECK:    [[TMP201:%.*]] = add i8 [[TMP199]], [[TMP200]]
; CHECK:    [[TMP202:%.*]] = xor i8 [[TMP199]], [[TMP200]]
; CHECK:    [[TMP203:%.*]] = xor i8 [[TMP201]], [[TMP199]]
; CHECK:    [[TMP204:%.*]] = xor i8 [[TMP202]], -1
; CHECK:    [[TMP205:%.*]] = and i8 [[TMP204]], [[TMP203]]
; CHECK:    [[TMP206:%.*]] = icmp slt i8 [[TMP205]], 0
; CHECK:    [[TMP207:%.*]] = icmp slt i8 0, [[TMP198]]
; CHECK:    [[TMP208:%.*]] = select i1 [[TMP207]], i8 127, i8 -128
; CHECK:    [[TMP209:%.*]] = select i1 [[TMP206]], i8 [[TMP208]], i8 [[TMP201]]
; CHECK:    [[TMP210:%.*]] = insertelement <16 x i8> [[TMP196]], i8 [[TMP209]], i64 14
; CHECK:    [[TMP211:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP212:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP213:%.*]] = bitcast i8 [[TMP211]] to i8
; CHECK:    [[TMP214:%.*]] = bitcast i8 [[TMP212]] to i8
; CHECK:    [[TMP215:%.*]] = add i8 [[TMP213]], [[TMP214]]
; CHECK:    [[TMP216:%.*]] = xor i8 [[TMP213]], [[TMP214]]
; CHECK:    [[TMP217:%.*]] = xor i8 [[TMP215]], [[TMP213]]
; CHECK:    [[TMP218:%.*]] = xor i8 [[TMP216]], -1
; CHECK:    [[TMP219:%.*]] = and i8 [[TMP218]], [[TMP217]]
; CHECK:    [[TMP220:%.*]] = icmp slt i8 [[TMP219]], 0
; CHECK:    [[TMP221:%.*]] = icmp slt i8 0, [[TMP212]]
; CHECK:    [[TMP222:%.*]] = select i1 [[TMP221]], i8 127, i8 -128
; CHECK:    [[TMP223:%.*]] = select i1 [[TMP220]], i8 [[TMP222]], i8 [[TMP215]]
; CHECK:    [[TMP224:%.*]] = insertelement <16 x i8> [[TMP210]], i8 [[TMP223]], i64 15
; CHECK:    ret <16 x i8> [[TMP224]]
;
  %1 = call <16 x i8> @llvm.sadd.sat.v16i8(<16 x i8> %s1, <16 x i8> %s2)
  ret <16 x i8> %1
}

define <16 x i8> @test_ssub_vec_i8(<16 x i8> %s1, <16 x i8> %s2) {
; CHECK-LABEL: define <16 x i8> @test_ssub_vec_i8(
; CHECK-SAME: <16 x i8> [[S1:%.*]], <16 x i8> [[S2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <16 x i8> [[S1]], i64 0
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[S2]], i64 0
; CHECK:    [[TMP3:%.*]] = bitcast i8 [[TMP1]] to i8
; CHECK:    [[TMP4:%.*]] = bitcast i8 [[TMP2]] to i8
; CHECK:    [[TMP5:%.*]] = sub i8 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = xor i8 [[TMP4]], -1
; CHECK:    [[TMP7:%.*]] = xor i8 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = xor i8 [[TMP5]], [[TMP3]]
; CHECK:    [[TMP9:%.*]] = xor i8 [[TMP7]], -1
; CHECK:    [[TMP10:%.*]] = and i8 [[TMP9]], [[TMP8]]
; CHECK:    [[TMP11:%.*]] = icmp slt i8 [[TMP10]], 0
; CHECK:    [[TMP12:%.*]] = icmp slt i8 0, [[TMP2]]
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP12]], i8 -128, i8 127
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP11]], i8 [[TMP13]], i8 [[TMP5]]
; CHECK:    [[TMP15:%.*]] = insertelement <16 x i8> poison, i8 [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = extractelement <16 x i8> [[S1]], i64 1
; CHECK:    [[TMP17:%.*]] = extractelement <16 x i8> [[S2]], i64 1
; CHECK:    [[TMP18:%.*]] = bitcast i8 [[TMP16]] to i8
; CHECK:    [[TMP19:%.*]] = bitcast i8 [[TMP17]] to i8
; CHECK:    [[TMP20:%.*]] = sub i8 [[TMP18]], [[TMP19]]
; CHECK:    [[TMP21:%.*]] = xor i8 [[TMP19]], -1
; CHECK:    [[TMP22:%.*]] = xor i8 [[TMP18]], [[TMP21]]
; CHECK:    [[TMP23:%.*]] = xor i8 [[TMP20]], [[TMP18]]
; CHECK:    [[TMP24:%.*]] = xor i8 [[TMP22]], -1
; CHECK:    [[TMP25:%.*]] = and i8 [[TMP24]], [[TMP23]]
; CHECK:    [[TMP26:%.*]] = icmp slt i8 [[TMP25]], 0
; CHECK:    [[TMP27:%.*]] = icmp slt i8 0, [[TMP17]]
; CHECK:    [[TMP28:%.*]] = select i1 [[TMP27]], i8 -128, i8 127
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP26]], i8 [[TMP28]], i8 [[TMP20]]
; CHECK:    [[TMP30:%.*]] = insertelement <16 x i8> [[TMP15]], i8 [[TMP29]], i64 1
; CHECK:    [[TMP31:%.*]] = extractelement <16 x i8> [[S1]], i64 2
; CHECK:    [[TMP32:%.*]] = extractelement <16 x i8> [[S2]], i64 2
; CHECK:    [[TMP33:%.*]] = bitcast i8 [[TMP31]] to i8
; CHECK:    [[TMP34:%.*]] = bitcast i8 [[TMP32]] to i8
; CHECK:    [[TMP35:%.*]] = sub i8 [[TMP33]], [[TMP34]]
; CHECK:    [[TMP36:%.*]] = xor i8 [[TMP34]], -1
; CHECK:    [[TMP37:%.*]] = xor i8 [[TMP33]], [[TMP36]]
; CHECK:    [[TMP38:%.*]] = xor i8 [[TMP35]], [[TMP33]]
; CHECK:    [[TMP39:%.*]] = xor i8 [[TMP37]], -1
; CHECK:    [[TMP40:%.*]] = and i8 [[TMP39]], [[TMP38]]
; CHECK:    [[TMP41:%.*]] = icmp slt i8 [[TMP40]], 0
; CHECK:    [[TMP42:%.*]] = icmp slt i8 0, [[TMP32]]
; CHECK:    [[TMP43:%.*]] = select i1 [[TMP42]], i8 -128, i8 127
; CHECK:    [[TMP44:%.*]] = select i1 [[TMP41]], i8 [[TMP43]], i8 [[TMP35]]
; CHECK:    [[TMP45:%.*]] = insertelement <16 x i8> [[TMP30]], i8 [[TMP44]], i64 2
; CHECK:    [[TMP46:%.*]] = extractelement <16 x i8> [[S1]], i64 3
; CHECK:    [[TMP47:%.*]] = extractelement <16 x i8> [[S2]], i64 3
; CHECK:    [[TMP48:%.*]] = bitcast i8 [[TMP46]] to i8
; CHECK:    [[TMP49:%.*]] = bitcast i8 [[TMP47]] to i8
; CHECK:    [[TMP50:%.*]] = sub i8 [[TMP48]], [[TMP49]]
; CHECK:    [[TMP51:%.*]] = xor i8 [[TMP49]], -1
; CHECK:    [[TMP52:%.*]] = xor i8 [[TMP48]], [[TMP51]]
; CHECK:    [[TMP53:%.*]] = xor i8 [[TMP50]], [[TMP48]]
; CHECK:    [[TMP54:%.*]] = xor i8 [[TMP52]], -1
; CHECK:    [[TMP55:%.*]] = and i8 [[TMP54]], [[TMP53]]
; CHECK:    [[TMP56:%.*]] = icmp slt i8 [[TMP55]], 0
; CHECK:    [[TMP57:%.*]] = icmp slt i8 0, [[TMP47]]
; CHECK:    [[TMP58:%.*]] = select i1 [[TMP57]], i8 -128, i8 127
; CHECK:    [[TMP59:%.*]] = select i1 [[TMP56]], i8 [[TMP58]], i8 [[TMP50]]
; CHECK:    [[TMP60:%.*]] = insertelement <16 x i8> [[TMP45]], i8 [[TMP59]], i64 3
; CHECK:    [[TMP61:%.*]] = extractelement <16 x i8> [[S1]], i64 4
; CHECK:    [[TMP62:%.*]] = extractelement <16 x i8> [[S2]], i64 4
; CHECK:    [[TMP63:%.*]] = bitcast i8 [[TMP61]] to i8
; CHECK:    [[TMP64:%.*]] = bitcast i8 [[TMP62]] to i8
; CHECK:    [[TMP65:%.*]] = sub i8 [[TMP63]], [[TMP64]]
; CHECK:    [[TMP66:%.*]] = xor i8 [[TMP64]], -1
; CHECK:    [[TMP67:%.*]] = xor i8 [[TMP63]], [[TMP66]]
; CHECK:    [[TMP68:%.*]] = xor i8 [[TMP65]], [[TMP63]]
; CHECK:    [[TMP69:%.*]] = xor i8 [[TMP67]], -1
; CHECK:    [[TMP70:%.*]] = and i8 [[TMP69]], [[TMP68]]
; CHECK:    [[TMP71:%.*]] = icmp slt i8 [[TMP70]], 0
; CHECK:    [[TMP72:%.*]] = icmp slt i8 0, [[TMP62]]
; CHECK:    [[TMP73:%.*]] = select i1 [[TMP72]], i8 -128, i8 127
; CHECK:    [[TMP74:%.*]] = select i1 [[TMP71]], i8 [[TMP73]], i8 [[TMP65]]
; CHECK:    [[TMP75:%.*]] = insertelement <16 x i8> [[TMP60]], i8 [[TMP74]], i64 4
; CHECK:    [[TMP76:%.*]] = extractelement <16 x i8> [[S1]], i64 5
; CHECK:    [[TMP77:%.*]] = extractelement <16 x i8> [[S2]], i64 5
; CHECK:    [[TMP78:%.*]] = bitcast i8 [[TMP76]] to i8
; CHECK:    [[TMP79:%.*]] = bitcast i8 [[TMP77]] to i8
; CHECK:    [[TMP80:%.*]] = sub i8 [[TMP78]], [[TMP79]]
; CHECK:    [[TMP81:%.*]] = xor i8 [[TMP79]], -1
; CHECK:    [[TMP82:%.*]] = xor i8 [[TMP78]], [[TMP81]]
; CHECK:    [[TMP83:%.*]] = xor i8 [[TMP80]], [[TMP78]]
; CHECK:    [[TMP84:%.*]] = xor i8 [[TMP82]], -1
; CHECK:    [[TMP85:%.*]] = and i8 [[TMP84]], [[TMP83]]
; CHECK:    [[TMP86:%.*]] = icmp slt i8 [[TMP85]], 0
; CHECK:    [[TMP87:%.*]] = icmp slt i8 0, [[TMP77]]
; CHECK:    [[TMP88:%.*]] = select i1 [[TMP87]], i8 -128, i8 127
; CHECK:    [[TMP89:%.*]] = select i1 [[TMP86]], i8 [[TMP88]], i8 [[TMP80]]
; CHECK:    [[TMP90:%.*]] = insertelement <16 x i8> [[TMP75]], i8 [[TMP89]], i64 5
; CHECK:    [[TMP91:%.*]] = extractelement <16 x i8> [[S1]], i64 6
; CHECK:    [[TMP92:%.*]] = extractelement <16 x i8> [[S2]], i64 6
; CHECK:    [[TMP93:%.*]] = bitcast i8 [[TMP91]] to i8
; CHECK:    [[TMP94:%.*]] = bitcast i8 [[TMP92]] to i8
; CHECK:    [[TMP95:%.*]] = sub i8 [[TMP93]], [[TMP94]]
; CHECK:    [[TMP96:%.*]] = xor i8 [[TMP94]], -1
; CHECK:    [[TMP97:%.*]] = xor i8 [[TMP93]], [[TMP96]]
; CHECK:    [[TMP98:%.*]] = xor i8 [[TMP95]], [[TMP93]]
; CHECK:    [[TMP99:%.*]] = xor i8 [[TMP97]], -1
; CHECK:    [[TMP100:%.*]] = and i8 [[TMP99]], [[TMP98]]
; CHECK:    [[TMP101:%.*]] = icmp slt i8 [[TMP100]], 0
; CHECK:    [[TMP102:%.*]] = icmp slt i8 0, [[TMP92]]
; CHECK:    [[TMP103:%.*]] = select i1 [[TMP102]], i8 -128, i8 127
; CHECK:    [[TMP104:%.*]] = select i1 [[TMP101]], i8 [[TMP103]], i8 [[TMP95]]
; CHECK:    [[TMP105:%.*]] = insertelement <16 x i8> [[TMP90]], i8 [[TMP104]], i64 6
; CHECK:    [[TMP106:%.*]] = extractelement <16 x i8> [[S1]], i64 7
; CHECK:    [[TMP107:%.*]] = extractelement <16 x i8> [[S2]], i64 7
; CHECK:    [[TMP108:%.*]] = bitcast i8 [[TMP106]] to i8
; CHECK:    [[TMP109:%.*]] = bitcast i8 [[TMP107]] to i8
; CHECK:    [[TMP110:%.*]] = sub i8 [[TMP108]], [[TMP109]]
; CHECK:    [[TMP111:%.*]] = xor i8 [[TMP109]], -1
; CHECK:    [[TMP112:%.*]] = xor i8 [[TMP108]], [[TMP111]]
; CHECK:    [[TMP113:%.*]] = xor i8 [[TMP110]], [[TMP108]]
; CHECK:    [[TMP114:%.*]] = xor i8 [[TMP112]], -1
; CHECK:    [[TMP115:%.*]] = and i8 [[TMP114]], [[TMP113]]
; CHECK:    [[TMP116:%.*]] = icmp slt i8 [[TMP115]], 0
; CHECK:    [[TMP117:%.*]] = icmp slt i8 0, [[TMP107]]
; CHECK:    [[TMP118:%.*]] = select i1 [[TMP117]], i8 -128, i8 127
; CHECK:    [[TMP119:%.*]] = select i1 [[TMP116]], i8 [[TMP118]], i8 [[TMP110]]
; CHECK:    [[TMP120:%.*]] = insertelement <16 x i8> [[TMP105]], i8 [[TMP119]], i64 7
; CHECK:    [[TMP121:%.*]] = extractelement <16 x i8> [[S1]], i64 8
; CHECK:    [[TMP122:%.*]] = extractelement <16 x i8> [[S2]], i64 8
; CHECK:    [[TMP123:%.*]] = bitcast i8 [[TMP121]] to i8
; CHECK:    [[TMP124:%.*]] = bitcast i8 [[TMP122]] to i8
; CHECK:    [[TMP125:%.*]] = sub i8 [[TMP123]], [[TMP124]]
; CHECK:    [[TMP126:%.*]] = xor i8 [[TMP124]], -1
; CHECK:    [[TMP127:%.*]] = xor i8 [[TMP123]], [[TMP126]]
; CHECK:    [[TMP128:%.*]] = xor i8 [[TMP125]], [[TMP123]]
; CHECK:    [[TMP129:%.*]] = xor i8 [[TMP127]], -1
; CHECK:    [[TMP130:%.*]] = and i8 [[TMP129]], [[TMP128]]
; CHECK:    [[TMP131:%.*]] = icmp slt i8 [[TMP130]], 0
; CHECK:    [[TMP132:%.*]] = icmp slt i8 0, [[TMP122]]
; CHECK:    [[TMP133:%.*]] = select i1 [[TMP132]], i8 -128, i8 127
; CHECK:    [[TMP134:%.*]] = select i1 [[TMP131]], i8 [[TMP133]], i8 [[TMP125]]
; CHECK:    [[TMP135:%.*]] = insertelement <16 x i8> [[TMP120]], i8 [[TMP134]], i64 8
; CHECK:    [[TMP136:%.*]] = extractelement <16 x i8> [[S1]], i64 9
; CHECK:    [[TMP137:%.*]] = extractelement <16 x i8> [[S2]], i64 9
; CHECK:    [[TMP138:%.*]] = bitcast i8 [[TMP136]] to i8
; CHECK:    [[TMP139:%.*]] = bitcast i8 [[TMP137]] to i8
; CHECK:    [[TMP140:%.*]] = sub i8 [[TMP138]], [[TMP139]]
; CHECK:    [[TMP141:%.*]] = xor i8 [[TMP139]], -1
; CHECK:    [[TMP142:%.*]] = xor i8 [[TMP138]], [[TMP141]]
; CHECK:    [[TMP143:%.*]] = xor i8 [[TMP140]], [[TMP138]]
; CHECK:    [[TMP144:%.*]] = xor i8 [[TMP142]], -1
; CHECK:    [[TMP145:%.*]] = and i8 [[TMP144]], [[TMP143]]
; CHECK:    [[TMP146:%.*]] = icmp slt i8 [[TMP145]], 0
; CHECK:    [[TMP147:%.*]] = icmp slt i8 0, [[TMP137]]
; CHECK:    [[TMP148:%.*]] = select i1 [[TMP147]], i8 -128, i8 127
; CHECK:    [[TMP149:%.*]] = select i1 [[TMP146]], i8 [[TMP148]], i8 [[TMP140]]
; CHECK:    [[TMP150:%.*]] = insertelement <16 x i8> [[TMP135]], i8 [[TMP149]], i64 9
; CHECK:    [[TMP151:%.*]] = extractelement <16 x i8> [[S1]], i64 10
; CHECK:    [[TMP152:%.*]] = extractelement <16 x i8> [[S2]], i64 10
; CHECK:    [[TMP153:%.*]] = bitcast i8 [[TMP151]] to i8
; CHECK:    [[TMP154:%.*]] = bitcast i8 [[TMP152]] to i8
; CHECK:    [[TMP155:%.*]] = sub i8 [[TMP153]], [[TMP154]]
; CHECK:    [[TMP156:%.*]] = xor i8 [[TMP154]], -1
; CHECK:    [[TMP157:%.*]] = xor i8 [[TMP153]], [[TMP156]]
; CHECK:    [[TMP158:%.*]] = xor i8 [[TMP155]], [[TMP153]]
; CHECK:    [[TMP159:%.*]] = xor i8 [[TMP157]], -1
; CHECK:    [[TMP160:%.*]] = and i8 [[TMP159]], [[TMP158]]
; CHECK:    [[TMP161:%.*]] = icmp slt i8 [[TMP160]], 0
; CHECK:    [[TMP162:%.*]] = icmp slt i8 0, [[TMP152]]
; CHECK:    [[TMP163:%.*]] = select i1 [[TMP162]], i8 -128, i8 127
; CHECK:    [[TMP164:%.*]] = select i1 [[TMP161]], i8 [[TMP163]], i8 [[TMP155]]
; CHECK:    [[TMP165:%.*]] = insertelement <16 x i8> [[TMP150]], i8 [[TMP164]], i64 10
; CHECK:    [[TMP166:%.*]] = extractelement <16 x i8> [[S1]], i64 11
; CHECK:    [[TMP167:%.*]] = extractelement <16 x i8> [[S2]], i64 11
; CHECK:    [[TMP168:%.*]] = bitcast i8 [[TMP166]] to i8
; CHECK:    [[TMP169:%.*]] = bitcast i8 [[TMP167]] to i8
; CHECK:    [[TMP170:%.*]] = sub i8 [[TMP168]], [[TMP169]]
; CHECK:    [[TMP171:%.*]] = xor i8 [[TMP169]], -1
; CHECK:    [[TMP172:%.*]] = xor i8 [[TMP168]], [[TMP171]]
; CHECK:    [[TMP173:%.*]] = xor i8 [[TMP170]], [[TMP168]]
; CHECK:    [[TMP174:%.*]] = xor i8 [[TMP172]], -1
; CHECK:    [[TMP175:%.*]] = and i8 [[TMP174]], [[TMP173]]
; CHECK:    [[TMP176:%.*]] = icmp slt i8 [[TMP175]], 0
; CHECK:    [[TMP177:%.*]] = icmp slt i8 0, [[TMP167]]
; CHECK:    [[TMP178:%.*]] = select i1 [[TMP177]], i8 -128, i8 127
; CHECK:    [[TMP179:%.*]] = select i1 [[TMP176]], i8 [[TMP178]], i8 [[TMP170]]
; CHECK:    [[TMP180:%.*]] = insertelement <16 x i8> [[TMP165]], i8 [[TMP179]], i64 11
; CHECK:    [[TMP181:%.*]] = extractelement <16 x i8> [[S1]], i64 12
; CHECK:    [[TMP182:%.*]] = extractelement <16 x i8> [[S2]], i64 12
; CHECK:    [[TMP183:%.*]] = bitcast i8 [[TMP181]] to i8
; CHECK:    [[TMP184:%.*]] = bitcast i8 [[TMP182]] to i8
; CHECK:    [[TMP185:%.*]] = sub i8 [[TMP183]], [[TMP184]]
; CHECK:    [[TMP186:%.*]] = xor i8 [[TMP184]], -1
; CHECK:    [[TMP187:%.*]] = xor i8 [[TMP183]], [[TMP186]]
; CHECK:    [[TMP188:%.*]] = xor i8 [[TMP185]], [[TMP183]]
; CHECK:    [[TMP189:%.*]] = xor i8 [[TMP187]], -1
; CHECK:    [[TMP190:%.*]] = and i8 [[TMP189]], [[TMP188]]
; CHECK:    [[TMP191:%.*]] = icmp slt i8 [[TMP190]], 0
; CHECK:    [[TMP192:%.*]] = icmp slt i8 0, [[TMP182]]
; CHECK:    [[TMP193:%.*]] = select i1 [[TMP192]], i8 -128, i8 127
; CHECK:    [[TMP194:%.*]] = select i1 [[TMP191]], i8 [[TMP193]], i8 [[TMP185]]
; CHECK:    [[TMP195:%.*]] = insertelement <16 x i8> [[TMP180]], i8 [[TMP194]], i64 12
; CHECK:    [[TMP196:%.*]] = extractelement <16 x i8> [[S1]], i64 13
; CHECK:    [[TMP197:%.*]] = extractelement <16 x i8> [[S2]], i64 13
; CHECK:    [[TMP198:%.*]] = bitcast i8 [[TMP196]] to i8
; CHECK:    [[TMP199:%.*]] = bitcast i8 [[TMP197]] to i8
; CHECK:    [[TMP200:%.*]] = sub i8 [[TMP198]], [[TMP199]]
; CHECK:    [[TMP201:%.*]] = xor i8 [[TMP199]], -1
; CHECK:    [[TMP202:%.*]] = xor i8 [[TMP198]], [[TMP201]]
; CHECK:    [[TMP203:%.*]] = xor i8 [[TMP200]], [[TMP198]]
; CHECK:    [[TMP204:%.*]] = xor i8 [[TMP202]], -1
; CHECK:    [[TMP205:%.*]] = and i8 [[TMP204]], [[TMP203]]
; CHECK:    [[TMP206:%.*]] = icmp slt i8 [[TMP205]], 0
; CHECK:    [[TMP207:%.*]] = icmp slt i8 0, [[TMP197]]
; CHECK:    [[TMP208:%.*]] = select i1 [[TMP207]], i8 -128, i8 127
; CHECK:    [[TMP209:%.*]] = select i1 [[TMP206]], i8 [[TMP208]], i8 [[TMP200]]
; CHECK:    [[TMP210:%.*]] = insertelement <16 x i8> [[TMP195]], i8 [[TMP209]], i64 13
; CHECK:    [[TMP211:%.*]] = extractelement <16 x i8> [[S1]], i64 14
; CHECK:    [[TMP212:%.*]] = extractelement <16 x i8> [[S2]], i64 14
; CHECK:    [[TMP213:%.*]] = bitcast i8 [[TMP211]] to i8
; CHECK:    [[TMP214:%.*]] = bitcast i8 [[TMP212]] to i8
; CHECK:    [[TMP215:%.*]] = sub i8 [[TMP213]], [[TMP214]]
; CHECK:    [[TMP216:%.*]] = xor i8 [[TMP214]], -1
; CHECK:    [[TMP217:%.*]] = xor i8 [[TMP213]], [[TMP216]]
; CHECK:    [[TMP218:%.*]] = xor i8 [[TMP215]], [[TMP213]]
; CHECK:    [[TMP219:%.*]] = xor i8 [[TMP217]], -1
; CHECK:    [[TMP220:%.*]] = and i8 [[TMP219]], [[TMP218]]
; CHECK:    [[TMP221:%.*]] = icmp slt i8 [[TMP220]], 0
; CHECK:    [[TMP222:%.*]] = icmp slt i8 0, [[TMP212]]
; CHECK:    [[TMP223:%.*]] = select i1 [[TMP222]], i8 -128, i8 127
; CHECK:    [[TMP224:%.*]] = select i1 [[TMP221]], i8 [[TMP223]], i8 [[TMP215]]
; CHECK:    [[TMP225:%.*]] = insertelement <16 x i8> [[TMP210]], i8 [[TMP224]], i64 14
; CHECK:    [[TMP226:%.*]] = extractelement <16 x i8> [[S1]], i64 15
; CHECK:    [[TMP227:%.*]] = extractelement <16 x i8> [[S2]], i64 15
; CHECK:    [[TMP228:%.*]] = bitcast i8 [[TMP226]] to i8
; CHECK:    [[TMP229:%.*]] = bitcast i8 [[TMP227]] to i8
; CHECK:    [[TMP230:%.*]] = sub i8 [[TMP228]], [[TMP229]]
; CHECK:    [[TMP231:%.*]] = xor i8 [[TMP229]], -1
; CHECK:    [[TMP232:%.*]] = xor i8 [[TMP228]], [[TMP231]]
; CHECK:    [[TMP233:%.*]] = xor i8 [[TMP230]], [[TMP228]]
; CHECK:    [[TMP234:%.*]] = xor i8 [[TMP232]], -1
; CHECK:    [[TMP235:%.*]] = and i8 [[TMP234]], [[TMP233]]
; CHECK:    [[TMP236:%.*]] = icmp slt i8 [[TMP235]], 0
; CHECK:    [[TMP237:%.*]] = icmp slt i8 0, [[TMP227]]
; CHECK:    [[TMP238:%.*]] = select i1 [[TMP237]], i8 -128, i8 127
; CHECK:    [[TMP239:%.*]] = select i1 [[TMP236]], i8 [[TMP238]], i8 [[TMP230]]
; CHECK:    [[TMP240:%.*]] = insertelement <16 x i8> [[TMP225]], i8 [[TMP239]], i64 15
; CHECK:    ret <16 x i8> [[TMP240]]
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