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

define void @test_scalarcg(<4 x i8> %src1, <3 x i32> %src2) {
; CHECK-LABEL: @test_scalarcg(
; CHECK:    [[TMP1:%.*]] = alloca <4 x i8>, align 1
; CHECK:    [[TMP2:%.*]] = alloca <3 x i32>, align 4
; CHECK:    [[TMP3:%.*]] = bitcast <4 x i8> [[SRC1:%.*]] to i32
; CHECK:    [[TMP4:%.*]] = and i32 bitcast (<4 x i8> <i8 1, i8 2, i8 3, i8 4> to i32), [[TMP3]]
; CHECK:    [[TMP5:%.*]] = bitcast i32 [[TMP4]] to <4 x i8>
; CHECK:    [[TMP6:%.*]] = extractelement <3 x i32> [[SRC2:%.*]], i32 0
; CHECK:    [[TMP7:%.*]] = or i32 13, [[TMP6]]
; CHECK:    [[TMP8:%.*]] = insertelement <3 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP9:%.*]] = extractelement <3 x i32> [[SRC2]], i32 1
; CHECK:    [[TMP10:%.*]] = or i32 42, [[TMP9]]
; CHECK:    [[TMP11:%.*]] = insertelement <3 x i32> [[TMP8]], i32 [[TMP10]], i32 1
; CHECK:    [[TMP12:%.*]] = extractelement <3 x i32> [[SRC2]], i32 2
; CHECK:    [[TMP13:%.*]] = or i32 17, [[TMP12]]
; CHECK:    [[TMP14:%.*]] = insertelement <3 x i32> [[TMP11]], i32 [[TMP13]], i32 2
; CHECK:    store <4 x i8> [[TMP5]], <4 x i8>* [[TMP1]], align 4
; CHECK:    store <3 x i32> [[TMP14]], <3 x i32>* [[TMP2]], align 16
; CHECK:    ret void
;
  %1 = alloca <4 x i8>, align 1
  %2 = alloca <3 x i32>, align 4
  %3 = and <4 x i8> <i8 1, i8 2, i8 3, i8 4>, %src1
  %4 = or <3 x i32> <i32 13, i32 42, i32 17>, %src2
  store <4 x i8> %3, <4 x i8>* %1, align 4
  store <3 x i32> %4, <3 x i32>* %2, align 16
  ret void
}
