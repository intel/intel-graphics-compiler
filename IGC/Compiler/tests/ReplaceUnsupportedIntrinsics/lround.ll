;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -instcombine -verify -S %s -o %t
; RUN: FileCheck %s < %t
; RUN: %if llvm-14-plus %{ igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -instcombine -verify -S %s -o %t %}
; RUN: %if llvm-14-plus %{ FileCheck %s < %t %}

; ===========================================================================
; Structural checks (parametric — verifies transformation pattern)
; ===========================================================================

define i32 @A(float %0) {
entry:
; CHECK-LABEL: @A
; CHECK:  [[RNDF:%[a-zA-Z0-9]+]] = call float @llvm.round.f32(float %0)
; CHECK:  [[CVTF:%[a-zA-Z0-9]+]] = fptosi float [[RNDF]] to i32
; CHECK:  ret i32 [[CVTF]]
  %1 = call i32 @llvm.lround.i32.f32(float %0)
  ret i32 %1
}

define i64 @B(double %0) {
entry:
; CHECK-LABEL: @B
; CHECK:  [[RNDD:%[a-zA-Z0-9]+]] = call double @llvm.round.f64(double %0)
; CHECK:  [[CVTD:%[a-zA-Z0-9]+]] = fptosi double [[RNDD]] to i64
; CHECK:  ret i64 [[CVTD]]
  %1 = call i64 @llvm.llround.i64.f64(double %0)
  ret i64 %1
}

define i32 @C(double %0) {
entry:
; CHECK-LABEL: @C
; CHECK:  [[RNDD32:%[a-zA-Z0-9]+]] = call double @llvm.round.f64(double %0)
; CHECK:  [[CVTD32:%[a-zA-Z0-9]+]] = fptosi double [[RNDD32]] to i32
; CHECK:  ret i32 [[CVTD32]]
  %1 = call i32 @llvm.lround.i32.f64(double %0)
  ret i32 %1
}

define i64 @D(float %0) {
entry:
; CHECK-LABEL: @D
; CHECK:  [[RNDF64:%[a-zA-Z0-9]+]] = call float @llvm.round.f32(float %0)
; CHECK:  [[CVTF64:%[a-zA-Z0-9]+]] = fptosi float [[RNDF64]] to i64
; CHECK:  ret i64 [[CVTF64]]
  %1 = call i64 @llvm.llround.i64.f32(float %0)
  ret i64 %1
}

; ===========================================================================
; Numerical value checks (constant folded — verifies correctness)
; llround(-0x1.fffffffffffffp-2) must return 0, not -1
; ===========================================================================

define i64 @test_llround_neg_near_half() {
; CHECK-LABEL: @test_llround_neg_near_half
; CHECK:  ret i64 0
  %1 = call i64 @llvm.llround.i64.f64(double 0xBFDFFFFFFFFFFFFF)
  ret i64 %1
}

define i64 @test_llround_pos_near_half() {
; CHECK-LABEL: @test_llround_pos_near_half
; CHECK:  ret i64 0
  %1 = call i64 @llvm.llround.i64.f64(double 0x3FDFFFFFFFFFFFFF)
  ret i64 %1
}

define i64 @test_llround_neg_half() {
; CHECK-LABEL: @test_llround_neg_half
; CHECK:  ret i64 -1
  %1 = call i64 @llvm.llround.i64.f64(double -5.000000e-01)
  ret i64 %1
}

define i64 @test_llround_pos_half() {
; CHECK-LABEL: @test_llround_pos_half
; CHECK:  ret i64 1
  %1 = call i64 @llvm.llround.i64.f64(double 5.000000e-01)
  ret i64 %1
}

define i64 @test_llround_neg_one_half() {
; CHECK-LABEL: @test_llround_neg_one_half
; CHECK:  ret i64 -2
  %1 = call i64 @llvm.llround.i64.f64(double -1.500000e+00)
  ret i64 %1
}

define i64 @test_llround_pos_one_half() {
; CHECK-LABEL: @test_llround_pos_one_half
; CHECK:  ret i64 2
  %1 = call i64 @llvm.llround.i64.f64(double 1.500000e+00)
  ret i64 %1
}

define i32 @test_lround_float_neg_near_half() {
; CHECK-LABEL: @test_lround_float_neg_near_half
; CHECK:  ret i32 0
  %1 = call i32 @llvm.lround.i32.f32(float 0xBFDFFFFFE0000000)
  ret i32 %1
}

declare i32 @llvm.lround.i32.f32(float)
declare i64 @llvm.llround.i64.f64(double)
declare i32 @llvm.lround.i32.f64(double)
declare i64 @llvm.llround.i64.f32(float)