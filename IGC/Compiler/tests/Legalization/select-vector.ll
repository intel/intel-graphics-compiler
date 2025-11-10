;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt -igc-legalization -S -dce --regkey=LegalizerScalarizeSelectInstructions=1 < %s | FileCheck %s
; ------------------------------------------------
; Legalization: select vector
; ------------------------------------------------

; Checks legalization of select with vector type
; into selects of individual elements

define <4 x i32> @test_select(i1 %cc, <4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_select(
; CHECK-SAME: i1 [[CC:%.*]], <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i32> [[SRC1]], i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[SRC2]], i32 0
; CHECK:    [[TMP3:%.*]] = select i1 [[CC]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i32> undef, i32 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP6:%.*]] = extractelement <4 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = select i1 [[CC]], i32 [[TMP5]], i32 [[TMP6]]
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i32> [[TMP4]], i32 [[TMP7]], i32 1
; CHECK:    [[TMP9:%.*]] = extractelement <4 x i32> [[SRC1]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <4 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP11:%.*]] = select i1 [[CC]], i32 [[TMP9]], i32 [[TMP10]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x i32> [[TMP8]], i32 [[TMP11]], i32 2
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[SRC1]], i32 3
; CHECK:    [[TMP14:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP15:%.*]] = select i1 [[CC]], i32 [[TMP13]], i32 [[TMP14]]
; CHECK:    [[TMP16:%.*]] = insertelement <4 x i32> [[TMP12]], i32 [[TMP15]], i32 3
; CHECK:    ret <4 x i32> [[TMP16]]
;
  %1 = select i1 %cc, <4 x i32> %src1, <4 x i32> %src2
  ret <4 x i32> %1
}

define <4 x i32> @test_select_vec(<4 x i1> %cc, <4 x i32> %src1, <4 x i32> %src2) {
; CHECK-LABEL: define <4 x i32> @test_select_vec(
; CHECK-SAME: <4 x i1> [[CC:%.*]], <4 x i32> [[SRC1:%.*]], <4 x i32> [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = extractelement <4 x i1> [[CC]], i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[SRC1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[SRC2]], i32 0
; CHECK:    [[TMP4:%.*]] = select i1 [[TMP1]], i32 [[TMP2]], i32 [[TMP3]]
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i32> undef, i32 [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <4 x i1> [[CC]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <4 x i32> [[SRC1]], i32 1
; CHECK:    [[TMP8:%.*]] = extractelement <4 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP6]], i32 [[TMP7]], i32 [[TMP8]]
; CHECK:    [[TMP10:%.*]] = insertelement <4 x i32> [[TMP5]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP11:%.*]] = extractelement <4 x i1> [[CC]], i32 2
; CHECK:    [[TMP12:%.*]] = extractelement <4 x i32> [[SRC1]], i32 2
; CHECK:    [[TMP13:%.*]] = extractelement <4 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP11]], i32 [[TMP12]], i32 [[TMP13]]
; CHECK:    [[TMP15:%.*]] = insertelement <4 x i32> [[TMP10]], i32 [[TMP14]], i32 2
; CHECK:    [[TMP16:%.*]] = extractelement <4 x i1> [[CC]], i32 3
; CHECK:    [[TMP17:%.*]] = extractelement <4 x i32> [[SRC1]], i32 3
; CHECK:    [[TMP18:%.*]] = extractelement <4 x i32> [[SRC2]], i32 3
; CHECK:    [[TMP19:%.*]] = select i1 [[TMP16]], i32 [[TMP17]], i32 [[TMP18]]
; CHECK:    [[TMP20:%.*]] = insertelement <4 x i32> [[TMP15]], i32 [[TMP19]], i32 3
; CHECK:    ret <4 x i32> [[TMP20]]
;
  %1 = select <4 x i1> %cc, <4 x i32> %src1, <4 x i32> %src2
  ret <4 x i32> %1
}

!igc.functions = !{!0, !1}

!0 = !{<4 x i32> (i1, <4 x i32>, <4 x i32>)* @test_select, !2}
!1 = !{<4 x i32> (<4 x i1>, <4 x i32>, <4 x i32>)* @test_select_vec, !2}
!2 = !{}
