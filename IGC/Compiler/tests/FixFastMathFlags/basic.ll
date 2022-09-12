;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --fix-fast-math-flags -S < %s | FileCheck %s
; ------------------------------------------------
; FixFastMathFlags
; ------------------------------------------------

; Flags modified
;
; CHECK: define void @test_fadd
; CHECK-NEXT: {{[A-z0-9]*}} = fadd
; CHECK-DAG: reassoc
; CHECK-DAG: nsz
; CHECK-DAG: afn
; CHECK-DAG: contract

define void @test_fadd(float %s1, float %s2) {
  %1 = fadd reassoc nsz float %s1, %s2
  call void @use_float(float %1)
  ret void
}

; Flags not modified
;
; CHECK: define void @test_fadd_not
; CHECK-NEXT: {{[A-z0-9]*}} = fdiv arcp float

define void @test_fadd_not(float %s1, float %s2) {
  %1 = fdiv arcp float %s1, %s2
  call void @use_float(float %1)
  ret void
}

; Modified flags for call
;
; CHECK: define void @test_call
; CHECK-NEXT: {{[A-z0-9]*}} = call
; CHECK-DAG: reassoc
; CHECK-DAG: arcp
; CHECK-DAG: afn
; CHECK-DAG: contract
; CHECK-SAME: sin.f32

define void @test_call(float %s1) {
  %1 = call reassoc arcp float @llvm.sin.f32(float %s1)
  call void @use_float(float %1)
  ret void
}

; fcmp drops flags
;
; CHECK: define void @test_fcmp1
; CHECK-NEXT: {{[A-z0-9]*}} = fcmp uno float

define void @test_fcmp1(float %s1) {
  %1 = fcmp ninf uno float %s1, 0.000000e+00
  call void @use_i1(i1 %1)
  ret void
}

; CHECK: define void @test_fcmp2
; CHECK-NEXT: {{[A-z0-9]*}} = fcmp une float

define void @test_fcmp2(float %s1) {
  %1 = fcmp fast une float %s1, %s1
  call void @use_i1(i1 %1)
  ret void
}

declare void @use_float(float);
declare void @use_i1(i1);
declare float @llvm.sin.f32(float)
