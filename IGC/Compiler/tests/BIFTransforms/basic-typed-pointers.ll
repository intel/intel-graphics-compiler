;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey EnableIntelFast=1 --igc-bif-transforms -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BIFTransforms
; ------------------------------------------------

; Test checks that bif functions are replaced by fast variants for float variants
; and half typed functions are not changed


define void @test_make_fast_length(float %src) {
; CHECK-LABEL: @test_make_fast_length(
; CHECK-NEXT:    [[TMP1:%.*]] = call float @_Z11fast_length.f32(float [[SRC:%.*]])
; CHECK-NEXT:    call void @use.f32(float [[TMP1]])
;
  %1 = call float @_Z6length.f32(float %src)
  call void @use.f32(float %1)
  ret void
}

define void @test_make_fast_distance(float %src1, float %src2) {
; CHECK-LABEL: @test_make_fast_distance(
; CHECK-NEXT:    [[TMP1:%.*]] = call float @_Z13fast_distance.f32(float [[SRC1:%.*]], float [[SRC2:%.*]])
; CHECK-NEXT:    call void @use.f32(float [[TMP1]])
;
  %1 = call float @_Z8distance.f32(float %src1, float %src2)
  call void @use.f32(float %1)
  ret void
}

define void @test_make_fast_normalize(float %src) {
; CHECK-LABEL: @test_make_fast_normalize(
; CHECK-NEXT:    [[TMP1:%.*]] = call float @_Z14fast_normalize.f32(float [[SRC:%.*]])
; CHECK-NEXT:    call void @use.f32(float [[TMP1]])
;
  %1 = call float @_Z9normalize.f32(float %src)
  call void @use.f32(float %1)
  ret void
}

define void @test_skip_half(half %src1, half %src2) {
; CHECK-LABEL: @test_skip_half(
; CHECK-NEXT:    [[TMP1:%.*]] = call half @_Z6length.f16(half [[SRC1:%.*]])
; CHECK-NEXT:    [[TMP2:%.*]] = call half @_Z8distance.f16(half [[SRC1]], half [[SRC2:%.*]])
; CHECK-NEXT:    [[TMP3:%.*]] = call half @_Z9normalize.f16(half [[SRC2]])
;
  %1 = call half @_Z6length.f16(half %src1)
  %2 = call half @_Z8distance.f16(half %src1, half %src2)
  %3 = call half @_Z9normalize.f16(half %src2)
  call void @use.f16(half %1)
  call void @use.f16(half %2)
  call void @use.f16(half %3)
  ret void
}

declare float @_Z6length.f32(float)
declare float @_Z8distance.f32(float, float)
declare float @_Z9normalize.f32(float)

declare half @_Z6length.f16(half)
declare half @_Z8distance.f16(half, half)
declare half @_Z9normalize.f16(half)

declare void @use.f32(float)
declare void @use.f16(half)
