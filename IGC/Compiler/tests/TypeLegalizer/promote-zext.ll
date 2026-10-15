;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-type-legalizer -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: zext from an illegal integer width to its promoted type
; ------------------------------------------------
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"

define i8 @test_add_i3(i8 %x, i8 %y) {
; CHECK-LABEL: define i8 @test_add_i3(
; CHECK:         [[ADD:%.*]] = add i8
; CHECK:         [[Z:%.*]] = and i8 [[ADD]], 7
; CHECK:         ret i8 [[Z]]
;
  %a = trunc i8 %x to i3
  %b = trunc i8 %y to i3
  %s = add i3 %a, %b
  %z = zext i3 %s to i8
  ret i8 %z
}

define i8 @test_ashr_i3(i8 %x) {
; CHECK-LABEL: define i8 @test_ashr_i3(
; CHECK:         [[ASHR:%.*]] = ashr i8 {{%.*}}, 1
; CHECK:         [[Z:%.*]] = and i8 [[ASHR]], 7
; CHECK:         ret i8 [[Z]]
;
  %a = trunc i8 %x to i3
  %s = ashr i3 %a, 1
  %z = zext i3 %s to i8
  ret i8 %z
}

define i8 @test_select_i3(i1 %c) {
; CHECK-LABEL: define i8 @test_select_i3(
; CHECK:         [[SEL:%.*]] = select i1 %c, i8 -1, i8 1
; CHECK:         [[Z:%.*]] = and i8 [[SEL]], 7
; CHECK:         ret i8 [[Z]]
;
  %s = select i1 %c, i3 -1, i3 1
  %z = zext i3 %s to i8
  ret i8 %z
}

define i32 @test_sub_i24(i32 %x) {
; CHECK-LABEL: define i32 @test_sub_i24(
; CHECK:         [[SUB:%.*]] = sub i32 0,
; CHECK:         [[Z:%.*]] = and i32 [[SUB]], 16777215
; CHECK:         ret i32 [[Z]]
;
  %a = trunc i32 %x to i24
  %s = sub i24 0, %a
  %z = zext i24 %s to i32
  ret i32 %z
}

define i64 @test_mul_i35(i64 %x) {
; CHECK-LABEL: define i64 @test_mul_i35(
; CHECK:         [[MUL:%.*]] = mul i64 {{%.*}}, 3
; CHECK:         [[Z:%.*]] = and i64 [[MUL]], 34359738367
; CHECK:         ret i64 [[Z]]
;
  %a = trunc i64 %x to i35
  %s = mul i35 %a, 3
  %z = zext i35 %s to i64
  ret i64 %z
}

define i16 @test_add_i3_to_i16(i8 %x, i8 %y) {
; CHECK-LABEL: define i16 @test_add_i3_to_i16(
; CHECK:         [[ADD:%.*]] = add i8
; CHECK:         [[M:%.*]] = and i8 [[ADD]], 7
; CHECK:         [[Z:%.*]] = zext i8 [[M]] to i16
; CHECK:         ret i16 [[Z]]
;
  %a = trunc i8 %x to i3
  %b = trunc i8 %y to i3
  %s = add i3 %a, %b
  %z = zext i3 %s to i16
  ret i16 %z
}
