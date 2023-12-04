;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -igc-evaluate-freeze -S < %s | FileCheck %s
; ------------------------------------------------
; EvaluateFreeze
; ------------------------------------------------

; Test check that freeze is substitued with magical values for undef and
; with value for definded.

define void @test_undef_i1() {
; CHECK-LABEL: @test_undef_i1(
; CHECK-NEXT:    call void @use.i1(i1 false)
; CHECK-NEXT:    ret void
;
  %1 = freeze i1 undef
  call void @use.i1(i1 %1)
  ret void
}

define void @test_undef_i16() {
; CHECK-LABEL: @test_undef_i16(
; CHECK-NEXT:    call void @use.i16(i16 -2946)
; CHECK-NEXT:    ret void
;
  %1 = freeze i16 undef
  call void @use.i16(i16 %1)
  ret void
}

define void @test_undef_i32() {
; CHECK-LABEL: @test_undef_i32(
; CHECK-NEXT:    call void @use.i32(i32 -185696768)
; CHECK-NEXT:    ret void
;
  %1 = freeze i32 undef
  call void @use.i32(i32 %1)
  ret void
}

define void @test_undef_i64() {
; CHECK-LABEL: @test_undef_i64(
; CHECK-NEXT:    call void @use.i64(i64 -797561541423628800)
; CHECK-NEXT:    ret void
;
  %1 = freeze i64 undef
  call void @use.i64(i64 %1)
  ret void
}

define void @test_f32() {
; CHECK-LABEL: @test_f32(
; CHECK-NEXT:    call void @use.f32(float 0.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = freeze float undef
  call void @use.f32(float %1)
  ret void
}

define void @test_def(float %a) {
; CHECK-LABEL: @test_def(
; CHECK-NEXT:    call void @use.f32(float %a)
; CHECK-NEXT:    ret void
;
  %1 = freeze float %a
  call void @use.f32(float %a)
  ret void
}

declare void @use.i1(i1)
declare void @use.i16(i16)
declare void @use.i32(i32)
declare void @use.i64(i64)
declare void @use.f32(float)
