;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-replace-unsupported-intrinsics -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; ReplaceUnsupportedIntrinsics: funnel shifts
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_shift(i32 %src1, i32 %src2, i32 %src3) {
; CHECK-LABEL: @test_shift(
; CHECK-NEXT:  entry:
; CHECK:    [[TMP0:%.*]] = and i32 [[SRC3:%.*]], 31
; CHECK:    [[TMP1:%.*]] = sub i32 32, [[TMP0]]
; CHECK:    [[TMP2:%.*]] = shl i32 [[SRC1:%.*]], [[TMP0]]
; CHECK:    [[TMP3:%.*]] = lshr i32 [[SRC2:%.*]], [[TMP1]]
; CHECK:    [[TMP4:%.*]] = or i32 [[TMP2]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = and i32 [[SRC3]], 31
; CHECK:    [[TMP6:%.*]] = sub i32 32, [[TMP5]]
; CHECK:    [[TMP7:%.*]] = shl i32 [[SRC1]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = lshr i32 [[SRC2]], [[TMP5]]
; CHECK:    [[TMP9:%.*]] = or i32 [[TMP7]], [[TMP8]]
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> undef, i32 [[SRC1]], i32 0
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> [[TMP10]], i32 [[SRC2]], i32 1
; CHECK:    [[TMP12:%.*]] = insertelement <2 x i32> undef, i32 [[SRC2]], i32 0
; CHECK:    [[TMP13:%.*]] = insertelement <2 x i32> [[TMP12]], i32 [[SRC3]], i32 1
; CHECK:    [[TMP14:%.*]] = insertelement <2 x i32> undef, i32 [[SRC3]], i32 0
; CHECK:    [[TMP15:%.*]] = insertelement <2 x i32> [[TMP14]], i32 [[SRC1]], i32 1
; CHECK:    [[TMP16:%.*]] = and <2 x i32> [[TMP15]], <i32 31, i32 31>
; CHECK:    [[TMP17:%.*]] = sub <2 x i32> <i32 32, i32 32>, [[TMP16]]
; CHECK:    [[TMP18:%.*]] = shl <2 x i32> [[TMP11]], [[TMP17]]
; CHECK:    [[TMP19:%.*]] = lshr <2 x i32> [[TMP13]], [[TMP16]]
; CHECK:    [[TMP20:%.*]] = or <2 x i32> [[TMP18]], [[TMP19]]
; CHECK:    ret void
;
entry:
  %fshl_result = call i32 @llvm.fshl.i32(i32 %src1, i32 %src2, i32 %src3)
  %fshr_result = call i32 @llvm.fshr.i32(i32 %src1, i32 %src2, i32 %src3)
  %0 = insertelement <2 x i32> undef, i32 %src1, i32 0
  %1 = insertelement <2 x i32> %0, i32 %src2, i32 1
  %2 = insertelement <2 x i32> undef, i32 %src2, i32 0
  %3 = insertelement <2 x i32> %2, i32 %src3, i32 1
  %4 = insertelement <2 x i32> undef, i32 %src3, i32 0
  %5 = insertelement <2 x i32> %4, i32 %src1, i32 1
  %fshr_vector_result = call <2 x i32> @llvm.fshr.v2i32(<2 x i32> %1, <2 x i32> %3, <2 x i32> %5)
  ret void
}

declare i32 @llvm.fshl.i32(i32, i32, i32) #0
declare i32 @llvm.fshr.i32(i32, i32, i32) #0
declare <2 x i32> @llvm.fshr.v2i32(<2 x i32>, <2 x i32>, <2 x i32>) #0
