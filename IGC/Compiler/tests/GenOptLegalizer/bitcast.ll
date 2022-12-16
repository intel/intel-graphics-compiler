;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -GenOptLegalizer -S < %s | FileCheck %s
; ------------------------------------------------
; GenOptLegalizer
; ------------------------------------------------

define void @bitcast_trunc_i48_f16(<3 x half> %src) {
; CHECK-LABEL: @bitcast_trunc_i48_f16(
; CHECK:    [[TMP1:%.*]] = extractelement <3 x half> %src, i32 0
; CHECK:    call void @use.f16(half [[TMP1]])
; CHECK:    ret void
;
  %1 = bitcast <3 x half> %src to i48
  %2 = trunc i48 %1 to i16
  %3 = bitcast i16 %2 to half
  call void @use.f16(half %3)
  ret void
}

define void @bitcast_trunc_lshr_i48_f16(<3 x half> %src) {
; CHECK-LABEL: @bitcast_trunc_lshr_i48_f16(
; CHECK:    [[TMP1:%.*]] = extractelement <3 x half> %src, i32 1
; CHECK:    call void @use.f16(half [[TMP1]])
; CHECK:    ret void
;
  %1 = bitcast <3 x half> %src to i48
  %2 = lshr i48 %1, 16
  %3 = trunc i48 %2 to i16
  %4 = bitcast i16 %3 to half
  call void @use.f16(half %4)
  ret void
}

define void @bitcast_trunc_i128_f32(<4 x float> %src) {
; CHECK-LABEL: @bitcast_trunc_i128_f32(
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> %src, i32 0
; CHECK:    [[TMP2:%.*]] = extractelement <4 x float> %src, i32 1
; CHECK:    [[TMP3:%.*]] = extractelement <4 x float> %src, i32 2
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    call void @use.f32(float [[TMP2]])
; CHECK:    call void @use.f32(float [[TMP3]])
; CHECK:    ret void
;
  %1 = bitcast <4 x float> %src to i128
  %2 = trunc i128 %1 to i96
  %3 = bitcast i96 %2 to <3 x float>
  %4 = extractelement <3 x float> %3, i32 0
  %5 = extractelement <3 x float> %3, i32 1
  %6 = extractelement <3 x float> %3, i32 2
  call void @use.f32(float %4)
  call void @use.f32(float %5)
  call void @use.f32(float %6)
  ret void
}

define void @bitcast_trunc_i128_i8(<4 x i32> %src) {
; CHECK-LABEL: @bitcast_trunc_i128_i8(
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> %src to <16 x i8>
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[TMP1]], i32 0
; CHECK:    call void @use.i8(i8 [[TMP2]])
; CHECK:    ret void
;
  %1 = bitcast <4 x i32> %src to i128
  %2 = trunc i128 %1 to i8
  call void @use.i8(i8 %2)
  ret void
}


define void @bitcast_trunc_lshr_i128_i8(<4 x i32> %src) {
; CHECK-LABEL: @bitcast_trunc_lshr_i128_i8(
; CHECK:    [[TMP1:%.*]] = bitcast <4 x i32> %src to <16 x i8>
; CHECK:    [[TMP2:%.*]] = extractelement <16 x i8> [[TMP1]], i32 1
; CHECK:    call void @use.i8(i8 [[TMP2]])
; CHECK:    ret void
;
  %1 = bitcast <4 x i32> %src to i128
  %2 = lshr i128 %1, 8
  %3 = trunc i128 %2 to i8
  call void @use.i8(i8 %3)
  ret void
}

declare void @use.i8(i8)
declare void @use.f16(half)
declare void @use.f32(float)
