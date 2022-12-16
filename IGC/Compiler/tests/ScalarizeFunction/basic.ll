;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------

define spir_kernel void @test_binary(<2 x i32> %src1, <2 x i32> %src2) {
; CHECK-LABEL: @test_binary(
; CHECK:    [[SCALAR2:%.*]] = extractelement <2 x i32> [[SRC2:%.*]], i32 0
; CHECK:    [[SCALAR3:%.*]] = extractelement <2 x i32> [[SRC2]], i32 1
; CHECK:    [[SCALAR:%.*]] = extractelement <2 x i32> [[SRC1:%.*]], i32 0
; CHECK:    [[SCALAR1:%.*]] = extractelement <2 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP1:%.*]] = alloca <2 x i32>, align 4
; CHECK:    [[TMP2:%.*]] = add i32 [[SCALAR]], [[SCALAR2]]
; CHECK:    [[TMP3:%.*]] = add i32 [[SCALAR1]], [[SCALAR3]]
; CHECK:    [[ASSEMBLED_VECT:%.*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[ASSEMBLED_VECT4:%.*]] = insertelement <2 x i32> [[ASSEMBLED_VECT]], i32 [[TMP3]], i32 1
; CHECK:    store <2 x i32> [[ASSEMBLED_VECT4]], <2 x i32>* [[TMP1]], align 8
; CHECK:    ret void
;
  %1 = alloca <2 x i32>, align 4
  %2 = add <2 x i32> %src1, %src2
  store <2 x i32> %2, <2 x i32>* %1, align 8
  ret void
}

define spir_kernel void @test_cast(<2 x i32> %src1) {
; CHECK-LABEL: @test_cast(
; CHECK:    [[SCALAR:%.*]] = extractelement <2 x i32> [[SRC1:%.*]], i32 0
; CHECK:    [[SCALAR1:%.*]] = extractelement <2 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP1:%.*]] = alloca <2 x i64>, align 4
; CHECK:    [[TMP2:%.*]] = alloca <4 x i16>, align 4
; CHECK:    [[TMP3:%.*]] = sext i32 [[SCALAR]] to i64
; CHECK:    [[TMP4:%.*]] = sext i32 [[SCALAR1]] to i64
; CHECK:    [[ASSEMBLED_VECT:%.*]] = insertelement <2 x i64> undef, i64 [[TMP3]], i32 0
; CHECK:    [[ASSEMBLED_VECT2:%.*]] = insertelement <2 x i64> [[ASSEMBLED_VECT]], i64 [[TMP4]], i32 1
; CHECK:    [[TMP5:%.*]] = bitcast <2 x i32> [[SRC1]] to <4 x i16>
; CHECK:    store <2 x i64> [[ASSEMBLED_VECT2]], <2 x i64>* [[TMP1]], align 16
; CHECK:    store <4 x i16> [[TMP5]], <4 x i16>* [[TMP2]], align 8
; CHECK:    ret void
;
  %1 = alloca <2 x i64>, align 4
  %2 = alloca <4 x i16>, align 4
  %3 = sext <2 x i32> %src1 to <2 x i64>
  %4 = bitcast <2 x i32> %src1 to <4 x i16>
  store <2 x i64> %3, <2 x i64>* %1, align 16
  store <4 x i16> %4, <4 x i16>* %2, align 8
  ret void
}

define spir_kernel void @test_cmp(<2 x i32> %src1, <2 x i32> %src2) {
; CHECK-LABEL: @test_cmp(
; CHECK:    [[SCALAR2:%.*]] = extractelement <2 x i32> [[SRC2:%.*]], i32 0
; CHECK:    [[SCALAR3:%.*]] = extractelement <2 x i32> [[SRC2]], i32 1
; CHECK:    [[SCALAR:%.*]] = extractelement <2 x i32> [[SRC1:%.*]], i32 0
; CHECK:    [[SCALAR1:%.*]] = extractelement <2 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP1:%.*]] = alloca <2 x i1>, align 4
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SCALAR]], [[SCALAR2]]
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SCALAR1]], [[SCALAR3]]
; CHECK:    [[ASSEMBLED_VECT:%.*]] = insertelement <2 x i1> undef, i1 [[TMP2]], i32 0
; CHECK:    [[ASSEMBLED_VECT4:%.*]] = insertelement <2 x i1> [[ASSEMBLED_VECT]], i1 [[TMP3]], i32 1
; CHECK:    store <2 x i1> [[ASSEMBLED_VECT4]], <2 x i1>* [[TMP1]], align 1
; CHECK:    ret void
;
  %1 = alloca <2 x i1>, align 4
  %2 = icmp eq <2 x i32> %src1, %src2
  store <2 x i1> %2, <2 x i1>* %1, align 1
  ret void
}

define spir_kernel void @test_select(<2 x i32> %src1, <4 x i16> %src2, i1 %cond, <4 x i1> %vcond) {
; CHECK-LABEL: @test_select(
; CHECK:    [[SCALAR6:%.*]] = extractelement <4 x i1> [[VCOND:%.*]], i32 0
; CHECK:    [[SCALAR7:%.*]] = extractelement <4 x i1> [[VCOND]], i32 1
; CHECK:    [[SCALAR8:%.*]] = extractelement <4 x i1> [[VCOND]], i32 2
; CHECK:    [[SCALAR9:%.*]] = extractelement <4 x i1> [[VCOND]], i32 3
; CHECK:    [[SCALAR2:%.*]] = extractelement <4 x i16> [[SRC2:%.*]], i32 0
; CHECK:    [[SCALAR3:%.*]] = extractelement <4 x i16> [[SRC2]], i32 1
; CHECK:    [[SCALAR4:%.*]] = extractelement <4 x i16> [[SRC2]], i32 2
; CHECK:    [[SCALAR5:%.*]] = extractelement <4 x i16> [[SRC2]], i32 3
; CHECK:    [[SCALAR:%.*]] = extractelement <2 x i32> [[SRC1:%.*]], i32 0
; CHECK:    [[SCALAR1:%.*]] = extractelement <2 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP1:%.*]] = alloca <2 x i32>, align 4
; CHECK:    [[TMP2:%.*]] = alloca <4 x i16>, align 4
; CHECK:    [[TMP3:%.*]] = select i1 [[COND:%.*]], i32 [[SCALAR]], i32 42
; CHECK:    [[TMP4:%.*]] = select i1 [[COND]], i32 [[SCALAR1]], i32 13
; CHECK:    [[ASSEMBLED_VECT:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i32 0
; CHECK:    [[ASSEMBLED_VECT10:%.*]] = insertelement <2 x i32> [[ASSEMBLED_VECT]], i32 [[TMP4]], i32 1
; CHECK:    [[TMP5:%.*]] = select i1 [[SCALAR6]], i16 [[SCALAR2]], i16 1
; CHECK:    [[TMP6:%.*]] = select i1 [[SCALAR7]], i16 [[SCALAR3]], i16 2
; CHECK:    [[TMP7:%.*]] = select i1 [[SCALAR8]], i16 [[SCALAR4]], i16 3
; CHECK:    [[TMP8:%.*]] = select i1 [[SCALAR9]], i16 [[SCALAR5]], i16 4
; CHECK:    [[ASSEMBLED_VECT11:%.*]] = insertelement <4 x i16> undef, i16 [[TMP5]], i32 0
; CHECK:    [[ASSEMBLED_VECT12:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT11]], i16 [[TMP6]], i32 1
; CHECK:    [[ASSEMBLED_VECT13:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT12]], i16 [[TMP7]], i32 2
; CHECK:    [[ASSEMBLED_VECT14:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT13]], i16 [[TMP8]], i32 3
; CHECK:    [[ASSEMBLED_VECT15:%.*]] = insertelement <4 x i16> undef, i16 [[SCALAR2]], i32 0
; CHECK:    [[ASSEMBLED_VECT16:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT15]], i16 [[SCALAR3]], i32 1
; CHECK:    [[ASSEMBLED_VECT17:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT16]], i16 [[SCALAR4]], i32 2
; CHECK:    [[ASSEMBLED_VECT18:%.*]] = insertelement <4 x i16> [[ASSEMBLED_VECT17]], i16 [[SCALAR5]], i32 3
; CHECK:    store <2 x i32> [[ASSEMBLED_VECT10]], <2 x i32>* [[TMP1]], align 8
; CHECK:    store <4 x i16> [[ASSEMBLED_VECT14]], <4 x i16>* [[TMP2]], align 8
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i16> [[ASSEMBLED_VECT18]] to i64
; CHECK:    ret void
;
  %1 = alloca <2 x i32>, align 4
  %2 = alloca <4 x i16>, align 4
  %3 = select i1 %cond, <2 x i32> %src1, <2 x i32> <i32 42, i32 13>
  %4 = select <4 x i1> %vcond, <4 x i16> %src2, <4 x i16> <i16 1, i16 2, i16 3, i16 4>
  %5 = select i1 %cond, <4 x i16> %src2, <4 x i16> %src2
  store <2 x i32> %3, <2 x i32>* %1, align 8
  store <4 x i16> %4, <4 x i16>* %2, align 8
  %6 = bitcast <4 x i16> %5 to i64
  ret void
}
