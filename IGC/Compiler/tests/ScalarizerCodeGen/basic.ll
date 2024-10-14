;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-scalarizer-in-codegen -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ScalarizerCodeGen
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; binary operators check

define <5 x i8> @test_scalarizer_binary_or(<5 x i8> %src1, <5 x i8> %src2) {
; CHECK-LABEL: define <5 x i8> @test_scalarizer_binary_or(
; CHECK-SAME: <5 x i8> [[SRC1:%.*]], <5 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i8> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <5 x i8> [[SRC2]], i32 0
; CHECK:    [[TMP3:%.*]] = or i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = insertelement <5 x i8> undef, i8 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <5 x i8> [[SRC1]], i32 1
; CHECK:    [[TMP6:%.*]] = extractelement <5 x i8> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = or i8 [[TMP5]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = insertelement <5 x i8> [[TMP4]], i8 [[TMP7]], i32 1
; CHECK:    [[TMP9:%.*]] = extractelement <5 x i8> [[SRC1]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i8> [[SRC2]], i32 2
; CHECK:    [[TMP11:%.*]] = or i8 [[TMP9]], [[TMP10]]
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i8> [[TMP8]], i8 [[TMP11]], i32 2
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i8> [[SRC1]], i32 3
; CHECK:    [[TMP14:%.*]] = extractelement <5 x i8> [[SRC2]], i32 3
; CHECK:    [[TMP15:%.*]] = or i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = insertelement <5 x i8> [[TMP12]], i8 [[TMP15]], i32 3
; CHECK:    [[TMP17:%.*]] = extractelement <5 x i8> [[SRC1]], i32 4
; CHECK:    [[TMP18:%.*]] = extractelement <5 x i8> [[SRC2]], i32 4
; CHECK:    [[TMP19:%.*]] = or i8 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = insertelement <5 x i8> [[TMP16]], i8 [[TMP19]], i32 4
; CHECK:    ret <5 x i8> [[TMP20]]
;
  %1 = or <5 x i8> %src1, %src2
  ret <5 x i8> %1
}

define <5 x i8> @test_scalarizer_binary_and(<5 x i8> %src1, <5 x i8> %src2) {
; CHECK-LABEL: define <5 x i8> @test_scalarizer_binary_and(
; CHECK-SAME: <5 x i8> [[SRC1:%.*]], <5 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i8> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <5 x i8> [[SRC2]], i32 0
; CHECK:    [[TMP3:%.*]] = and i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = insertelement <5 x i8> undef, i8 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <5 x i8> [[SRC1]], i32 1
; CHECK:    [[TMP6:%.*]] = extractelement <5 x i8> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = and i8 [[TMP5]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = insertelement <5 x i8> [[TMP4]], i8 [[TMP7]], i32 1
; CHECK:    [[TMP9:%.*]] = extractelement <5 x i8> [[SRC1]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i8> [[SRC2]], i32 2
; CHECK:    [[TMP11:%.*]] = and i8 [[TMP9]], [[TMP10]]
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i8> [[TMP8]], i8 [[TMP11]], i32 2
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i8> [[SRC1]], i32 3
; CHECK:    [[TMP14:%.*]] = extractelement <5 x i8> [[SRC2]], i32 3
; CHECK:    [[TMP15:%.*]] = and i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = insertelement <5 x i8> [[TMP12]], i8 [[TMP15]], i32 3
; CHECK:    [[TMP17:%.*]] = extractelement <5 x i8> [[SRC1]], i32 4
; CHECK:    [[TMP18:%.*]] = extractelement <5 x i8> [[SRC2]], i32 4
; CHECK:    [[TMP19:%.*]] = and i8 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = insertelement <5 x i8> [[TMP16]], i8 [[TMP19]], i32 4
; CHECK:    ret <5 x i8> [[TMP20]]
;
  %1 = and <5 x i8> %src1, %src2
  ret <5 x i8> %1
}

define <5 x i8> @test_scalarizer_binary_xor(<5 x i8> %src1, <5 x i8> %src2) {
; CHECK-LABEL: define <5 x i8> @test_scalarizer_binary_xor(
; CHECK-SAME: <5 x i8> [[SRC1:%.*]], <5 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i8> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <5 x i8> [[SRC2]], i32 0
; CHECK:    [[TMP3:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = insertelement <5 x i8> undef, i8 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <5 x i8> [[SRC1]], i32 1
; CHECK:    [[TMP6:%.*]] = extractelement <5 x i8> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = xor i8 [[TMP5]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = insertelement <5 x i8> [[TMP4]], i8 [[TMP7]], i32 1
; CHECK:    [[TMP9:%.*]] = extractelement <5 x i8> [[SRC1]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i8> [[SRC2]], i32 2
; CHECK:    [[TMP11:%.*]] = xor i8 [[TMP9]], [[TMP10]]
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i8> [[TMP8]], i8 [[TMP11]], i32 2
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i8> [[SRC1]], i32 3
; CHECK:    [[TMP14:%.*]] = extractelement <5 x i8> [[SRC2]], i32 3
; CHECK:    [[TMP15:%.*]] = xor i8 [[TMP13]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = insertelement <5 x i8> [[TMP12]], i8 [[TMP15]], i32 3
; CHECK:    [[TMP17:%.*]] = extractelement <5 x i8> [[SRC1]], i32 4
; CHECK:    [[TMP18:%.*]] = extractelement <5 x i8> [[SRC2]], i32 4
; CHECK:    [[TMP19:%.*]] = xor i8 [[TMP17]], [[TMP18]]
; CHECK:    [[TMP20:%.*]] = insertelement <5 x i8> [[TMP16]], i8 [[TMP19]], i32 4
; CHECK:    ret <5 x i8> [[TMP20]]
;
  %1 = xor <5 x i8> %src1, %src2
  ret <5 x i8> %1
}

; bitcast-able to scalar

define <8 x i1> @test_scalarizer_binary_i8(<8 x i1> %src1, <8 x i1> %src2) {
; CHECK-LABEL: define <8 x i1> @test_scalarizer_binary_i8(
; CHECK-SAME: <8 x i1> [[SRC1:%.*]], <8 x i1> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = bitcast <8 x i1> [[SRC1]] to i8
; CHECK:    [[TMP2:%.*]] = bitcast <8 x i1> [[SRC2]] to i8
; CHECK:    [[TMP3:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = bitcast i8 [[TMP3]] to <8 x i1>
; CHECK:    ret <8 x i1> [[TMP4]]
;
  %1 = xor <8 x i1> %src1, %src2
  ret <8 x i1> %1
}

define <2 x i8> @test_scalarizer_binary_i16(<2 x i8> %src1, <2 x i8> %src2) {
; CHECK-LABEL: define <2 x i8> @test_scalarizer_binary_i16(
; CHECK-SAME: <2 x i8> [[SRC1:%.*]], <2 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i8> [[SRC1]] to i16
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i8> [[SRC2]] to i16
; CHECK:    [[TMP3:%.*]] = xor i16 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = bitcast i16 [[TMP3]] to <2 x i8>
; CHECK:    ret <2 x i8> [[TMP4]]
;
  %1 = xor <2 x i8> %src1, %src2
  ret <2 x i8> %1
}

define <4 x i8> @test_scalarizer_binary_i32(<4 x i8> %src1, <4 x i8> %src2) {
; CHECK-LABEL: define <4 x i8> @test_scalarizer_binary_i32(
; CHECK-SAME: <4 x i8> [[SRC1:%.*]], <4 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i8> [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i8> [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = xor i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <4 x i8>
; CHECK:    ret <4 x i8> [[TMP4]]
;
  %1 = xor <4 x i8> %src1, %src2
  ret <4 x i8> %1
}

; bitcast-able to vector

define <6 x i8> @test_scalarizer_binary_v3i16(<6 x i8> %src1, <6 x i8> %src2) {
; CHECK-LABEL: define <6 x i8> @test_scalarizer_binary_v3i16(
; CHECK-SAME: <6 x i8> [[SRC1:%.*]], <6 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = bitcast <6 x i8> [[SRC1]] to <3 x i16>
; CHECK:    [[TMP2:%.*]] = bitcast <6 x i8> [[SRC2]] to <3 x i16>
; CHECK:    [[TMP3:%.*]] = extractelement <3 x i16> [[TMP1]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <3 x i16> [[TMP2]], i32 0
; CHECK:    [[TMP5:%.*]] = xor i16 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = insertelement <3 x i16> undef, i16 [[TMP5]], i32 0
; CHECK:    [[TMP7:%.*]] = extractelement <3 x i16> [[TMP1]], i32 1
; CHECK:    [[TMP8:%.*]] = extractelement <3 x i16> [[TMP2]], i32 1
; CHECK:    [[TMP9:%.*]] = xor i16 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = insertelement <3 x i16> [[TMP6]], i16 [[TMP9]], i32 1
; CHECK:    [[TMP11:%.*]] = extractelement <3 x i16> [[TMP1]], i32 2
; CHECK:    [[TMP12:%.*]] = extractelement <3 x i16> [[TMP2]], i32 2
; CHECK:    [[TMP13:%.*]] = xor i16 [[TMP11]], [[TMP12]]
; CHECK:    [[TMP14:%.*]] = insertelement <3 x i16> [[TMP10]], i16 [[TMP13]], i32 2
; CHECK:    [[TMP15:%.*]] = bitcast <3 x i16> [[TMP14]] to <6 x i8>
; CHECK:    ret <6 x i8> [[TMP15]]
;
  %1 = xor <6 x i8> %src1, %src2
  ret <6 x i8> %1
}

define <1 x i64> @test_scalarizer_binary_v2i32(<1 x i64> %src1, <1 x i64> %src2) {
; CHECK-LABEL: define <1 x i64> @test_scalarizer_binary_v2i32(
; CHECK-SAME: <1 x i64> [[SRC1:%.*]], <1 x i64> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = bitcast <1 x i64> [[SRC1]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = bitcast <1 x i64> [[SRC2]] to <2 x i32>
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK:    [[TMP5:%.*]] = xor i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i32> undef, i32 [[TMP5]], i32 0
; CHECK:    [[TMP7:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP8:%.*]] = extractelement <2 x i32> [[TMP2]], i32 1
; CHECK:    [[TMP9:%.*]] = xor i32 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to <1 x i64>
; CHECK:    ret <1 x i64> [[TMP11]]
;
  %1 = xor <1 x i64> %src1, %src2
  ret <1 x i64> %1
}

; sanity(not optimized)

define <5 x i8> @test_scalarizer_binary_add(<5 x i8> %src1, <5 x i8> %src2) {
; CHECK-LABEL: define <5 x i8> @test_scalarizer_binary_add(
; CHECK-SAME: <5 x i8> [[SRC1:%.*]], <5 x i8> [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = add <5 x i8> [[SRC1]], [[SRC2]]
; CHECK:    ret <5 x i8> [[TMP1]]
;
  %1 = add <5 x i8> %src1, %src2
  ret <5 x i8> %1
}

define i64 @test_scalarizer_binary_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_scalarizer_binary_i64(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = or i64 [[SRC1]], [[SRC2]]
; CHECK:    ret i64 [[TMP1]]
;
  %1 = or i64 %src1, %src2
  ret i64 %1
}
