;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-vector-bitcast-opt -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; VectorBitCastOpt
; ------------------------------------------------

; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 1
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vbitcast(<2 x i32> %src) {
; CHECK-LABEL: @test_vbitcast(
; CHECK:    [[TMP1:%.*]] = extractelement <2 x i32> [[SRC:%.*]], i32 1
; CHECK:    [[TMP2:%.*]] = bitcast i32 [[TMP1]] to float
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[SRC]], i32 2
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to float
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[SRC]], i32 1
; CHECK:    call void @use.i32(i32 [[TMP5]])
; CHECK:    call void @use.f32(float [[TMP2]])
; CHECK:    call void @use.f32(float [[TMP4]])
; CHECK:    ret void
;
  %1 = bitcast <2 x i32> %src to <2 x float>
  %2 = extractelement <2 x float> %1, i32 1
  %3 = extractelement <2 x float> %1, i32 2
  %4 = extractelement <2 x i32> %src, i32 1
  call void @use.i32(i32 %4)
  call void @use.f32(float %2)
  call void @use.f32(float %3)
  ret void
}

declare void @use.i32(i32)
declare void @use.f32(float)
