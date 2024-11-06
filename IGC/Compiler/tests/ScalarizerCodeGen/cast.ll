;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
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

; cast operators check

define <5 x i16> @test_scalarizer_zext(<5 x i8> %src1) {
; CHECK-LABEL: define <5 x i16> @test_scalarizer_zext(
; CHECK-SAME: <5 x i8> [[SRC1:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i8> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = zext i8 [[TMP1]] to i16
; CHECK:    [[TMP3:%.*]] = insertelement <5 x i16> undef, i16 [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <5 x i8> [[SRC1]], i32 1
; CHECK:    [[TMP5:%.*]] = zext i8 [[TMP4]] to i16
; CHECK:    [[TMP6:%.*]] = insertelement <5 x i16> [[TMP3]], i16 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <5 x i8> [[SRC1]], i32 2
; CHECK:    [[TMP8:%.*]] = zext i8 [[TMP7]] to i16
; CHECK:    [[TMP9:%.*]] = insertelement <5 x i16> [[TMP6]], i16 [[TMP8]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i8> [[SRC1]], i32 3
; CHECK:    [[TMP11:%.*]] = zext i8 [[TMP10]] to i16
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i16> [[TMP9]], i16 [[TMP11]], i32 3
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i8> [[SRC1]], i32 4
; CHECK:    [[TMP14:%.*]] = zext i8 [[TMP13]] to i16
; CHECK:    [[TMP15:%.*]] = insertelement <5 x i16> [[TMP12]], i16 [[TMP14]], i32 4
; CHECK:    ret <5 x i16> [[TMP15]]
;
  %1 = zext <5 x i8> %src1 to <5 x i16>
  ret <5 x i16> %1
}

define <5 x i8> @test_scalarizer_sext(<5 x i1> %src1) {
; CHECK-LABEL: define <5 x i8> @test_scalarizer_sext(
; CHECK-SAME: <5 x i1> [[SRC1:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i1> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = sext i1 [[TMP1]] to i8
; CHECK:    [[TMP3:%.*]] = insertelement <5 x i8> undef, i8 [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <5 x i1> [[SRC1]], i32 1
; CHECK:    [[TMP5:%.*]] = sext i1 [[TMP4]] to i8
; CHECK:    [[TMP6:%.*]] = insertelement <5 x i8> [[TMP3]], i8 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <5 x i1> [[SRC1]], i32 2
; CHECK:    [[TMP8:%.*]] = sext i1 [[TMP7]] to i8
; CHECK:    [[TMP9:%.*]] = insertelement <5 x i8> [[TMP6]], i8 [[TMP8]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i1> [[SRC1]], i32 3
; CHECK:    [[TMP11:%.*]] = sext i1 [[TMP10]] to i8
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i8> [[TMP9]], i8 [[TMP11]], i32 3
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i1> [[SRC1]], i32 4
; CHECK:    [[TMP14:%.*]] = sext i1 [[TMP13]] to i8
; CHECK:    [[TMP15:%.*]] = insertelement <5 x i8> [[TMP12]], i8 [[TMP14]], i32 4
; CHECK:    ret <5 x i8> [[TMP15]]
;
  %1 = sext <5 x i1> %src1 to <5 x i8>
  ret <5 x i8> %1
}

define <5 x i1> @test_scalarizer_trunc(<5 x i32> %src1) {
; CHECK-LABEL: define <5 x i1> @test_scalarizer_trunc(
; CHECK-SAME: <5 x i32> [[SRC1:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <5 x i32> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = trunc i32 [[TMP1]] to i1
; CHECK:    [[TMP3:%.*]] = insertelement <5 x i1> undef, i1 [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <5 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP5:%.*]] = trunc i32 [[TMP4]] to i1
; CHECK:    [[TMP6:%.*]] = insertelement <5 x i1> [[TMP3]], i1 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <5 x i32> [[SRC1]], i32 2
; CHECK:    [[TMP8:%.*]] = trunc i32 [[TMP7]] to i1
; CHECK:    [[TMP9:%.*]] = insertelement <5 x i1> [[TMP6]], i1 [[TMP8]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <5 x i32> [[SRC1]], i32 3
; CHECK:    [[TMP11:%.*]] = trunc i32 [[TMP10]] to i1
; CHECK:    [[TMP12:%.*]] = insertelement <5 x i1> [[TMP9]], i1 [[TMP11]], i32 3
; CHECK:    [[TMP13:%.*]] = extractelement <5 x i32> [[SRC1]], i32 4
; CHECK:    [[TMP14:%.*]] = trunc i32 [[TMP13]] to i1
; CHECK:    [[TMP15:%.*]] = insertelement <5 x i1> [[TMP12]], i1 [[TMP14]], i32 4
; CHECK:    ret <5 x i1> [[TMP15]]
;
  %1 = trunc <5 x i32> %src1 to <5 x i1>
  ret <5 x i1> %1
}

; sanity(not optimized)

define <5 x float> @test_scalarizer_fptrunc(<5 x double> %src1) {
; CHECK-LABEL: define <5 x float> @test_scalarizer_fptrunc(
; CHECK-SAME: <5 x double> [[SRC1:%.*]])
; CHECK:    [[TMP1:%.*]] = fptrunc <5 x double> [[SRC1]] to <5 x float>
; CHECK:    ret <5 x float> [[TMP1]]
;
  %1 = fptrunc <5 x double> %src1 to <5 x float>
  ret <5 x float> %1
}

define i32 @test_scalarizer_i64(i64 %src1) {
; CHECK-LABEL: define i32 @test_scalarizer_i64(
; CHECK-SAME: i64 [[SRC1:%.*]])
; CHECK:    [[TMP1:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK:    ret i32 [[TMP1]]
;
  %1 = trunc i64 %src1 to i32
  ret i32 %1
}
