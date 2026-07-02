;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: WaveAll with constant operands
; ------------------------------------------------
; WaveOps enum: SUM=0, PROD=1, UMIN=2, UMAX=3, IMIN=4, IMAX=5, OR=6, XOR=7, AND=8, FSUM=9, FPROD=10, FMIN=11, FMAX=12

; --- Integer idempotent ops: WaveAll(C, MIN/MAX/AND/OR) -> C ---

define void @test_waveall_const_umin() {
; CHECK-LABEL: @test_waveall_const_umin(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 2, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_const_umax() {
; CHECK-LABEL: @test_waveall_const_umax(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 3, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_const_imin() {
; CHECK-LABEL: @test_waveall_const_imin(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 4, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_const_imax() {
; CHECK-LABEL: @test_waveall_const_imax(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 5, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_const_and() {
; CHECK-LABEL: @test_waveall_const_and(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 8, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_const_or() {
; CHECK-LABEL: @test_waveall_const_or(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 6, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; --- Integer zero with SUM/XOR -> 0 ---

define void @test_waveall_zero_sum() {
; CHECK-LABEL: @test_waveall_zero_sum(
; CHECK-NEXT:    call void @use.i32(i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 0, i8 0, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_zero_xor() {
; CHECK-LABEL: @test_waveall_zero_xor(
; CHECK-NEXT:    call void @use.i32(i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 0, i8 7, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; --- Integer PROD: 0 -> 0 (absorbing), 1 -> 1 (identity) ---

define void @test_waveall_zero_prod() {
; CHECK-LABEL: @test_waveall_zero_prod(
; CHECK-NEXT:    call void @use.i32(i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 0, i8 1, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_waveall_one_prod() {
; CHECK-LABEL: @test_waveall_one_prod(
; CHECK-NEXT:    call void @use.i32(i32 1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 1, i8 1, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; --- Float idempotent ops: WaveAll(C, FMIN/FMAX) -> C ---

define void @test_waveall_const_fmin() {
; CHECK-LABEL: @test_waveall_const_fmin(
; CHECK-NEXT:    call void @use.f32(float 3.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 3.000000e+00, i8 11, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

define void @test_waveall_const_fmax() {
; CHECK-LABEL: @test_waveall_const_fmax(
; CHECK-NEXT:    call void @use.f32(float 3.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 3.000000e+00, i8 12, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

; --- Float zero with FSUM -> 0.0 ---

define void @test_waveall_zero_fsum() {
; CHECK-LABEL: @test_waveall_zero_fsum(
; CHECK-NEXT:    call void @use.f32(float 0.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 9, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

; --- Float FPROD: 0.0 -> 0.0 (absorbing), 1.0 -> 1.0 (identity) ---

define void @test_waveall_zero_fprod() {
; CHECK-LABEL: @test_waveall_zero_fprod(
; CHECK-NEXT:    call void @use.f32(float 0.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 0.000000e+00, i8 10, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

define void @test_waveall_one_fprod() {
; CHECK-LABEL: @test_waveall_one_fprod(
; CHECK-NEXT:    call void @use.f32(float 1.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 1.000000e+00, i8 10, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

; --- Negative cases: should NOT be folded ---

; Non-zero integer with SUM should not fold.
define void @test_waveall_nonzero_sum_no_fold() {
; CHECK-LABEL: @test_waveall_nonzero_sum_no_fold(
; CHECK:         %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 0, i1 false, i32 0)
; CHECK-NEXT:    call void @use.i32(i32 %1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 42, i8 0, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; Non-zero, non-one integer with PROD should not fold.
define void @test_waveall_nonzero_nonone_prod_no_fold() {
; CHECK-LABEL: @test_waveall_nonzero_nonone_prod_no_fold(
; CHECK:         %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 5, i8 1, i1 false, i32 0)
; CHECK-NEXT:    call void @use.i32(i32 %1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 5, i8 1, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; Non-zero float with FSUM should not fold.
define void @test_waveall_nonzero_fsum_no_fold() {
; CHECK-LABEL: @test_waveall_nonzero_fsum_no_fold(
; CHECK:         %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 3.000000e+00, i8 9, i1 false, i32 0)
; CHECK-NEXT:    call void @use.f32(float %1)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 3.000000e+00, i8 9, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

; Non-zero, non-one float with FPROD should not fold.
define void @test_waveall_nonzero_nonone_fprod_no_fold() {
; CHECK-LABEL: @test_waveall_nonzero_nonone_fprod_no_fold(
; CHECK:         %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 2.000000e+00, i8 10, i1 false, i32 0)
; CHECK-NEXT:    call void @use.f32(float %1)
; CHECK-NEXT:    ret void
;
  %1 = call float @llvm.genx.GenISA.WaveAll.f32(float 2.000000e+00, i8 10, i1 false, i32 0)
  call void @use.f32(float %1)
  ret void
}

; Non-constant source should not fold.
define void @test_waveall_nonconstant_no_fold(i32 %x) {
; CHECK-LABEL: @test_waveall_nonconstant_no_fold(
; CHECK:         %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %x, i8 2, i1 false, i32 0)
; CHECK-NEXT:    call void @use.i32(i32 %1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %x, i8 2, i1 false, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; Vector source (joint-reduction) should not fold.
define void @test_waveall_vector_no_fold() {
; CHECK-LABEL: @test_waveall_vector_no_fold(
; CHECK:         %1 = call <2 x i32> @llvm.genx.GenISA.WaveAll.v2i32(<2 x i32> zeroinitializer, i8 0, i1 false, i32 0)
; CHECK-NEXT:    call void @use.v2i32(<2 x i32> %1)
; CHECK-NEXT:    ret void
;
  %1 = call <2 x i32> @llvm.genx.GenISA.WaveAll.v2i32(<2 x i32> zeroinitializer, i8 0, i1 false, i32 0)
  call void @use.v2i32(<2 x i32> %1)
  ret void
}

declare i32 @llvm.genx.GenISA.WaveAll.i32(i32, i8, i1, i32)
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32)
declare <2 x i32> @llvm.genx.GenISA.WaveAll.v2i32(<2 x i32>, i8, i1, i32)
declare void @use.i32(i32)
declare void @use.f32(float)
declare void @use.v2i32(<2 x i32>)
