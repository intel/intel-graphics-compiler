;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -instcombine -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: WaveShuffleIndex with constant source operand
; ------------------------------------------------
; When the value being shuffled (operand 0) is a constant (int or fp),
; WaveShuffleIndex(C, lane, helper) simplifies directly to C, regardless
; of the lane index, since every lane holds the same constant value.

; --- Integer constant source: WaveShuffleIndex(C, lane, 0) -> C ---

define void @test_waveshuffle_const_int(i32 %lane) {
; CHECK-LABEL: @test_waveshuffle_const_int(
; CHECK-NEXT:    call void @use.i32(i32 42)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 42, i32 %lane, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

; --- FP constant source: WaveShuffleIndex(C, lane, 0) -> C ---

define void @test_waveshuffle_const_fp(i32 %lane) {
; CHECK-LABEL: @test_waveshuffle_const_fp(
; CHECK-NEXT:    call void @use.f32(float 3.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call fast float @llvm.genx.GenISA.WaveShuffleIndex.f32(float 3.000000e+00, i32 %lane, i32 0)
  call void @use.f32(float %1)
  ret void
}

; --- Zero constant, chained/reused shuffle (mirrors real-world pattern) ---
; %251 = WaveShuffleIndex(0.0, %244, 0)          -> folds to 0.0
; %253 = WaveShuffleIndex(%251(=0.0), %252, 0)   -> folds to 0.0
; %254 = fadd(%253(=0.0), %251(=0.0))            -> should be foldable/simplified downstream,
;         but at minimum both shuffles must fold away.

define void @test_waveshuffle_zero_fp_propagates(i32 %lane_a, i32 %lane_b) {
; CHECK-LABEL: @test_waveshuffle_zero_fp_propagates(
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveShuffleIndex.f32
; CHECK:         call void @use.f32(float 0.000000e+00)
; CHECK-NEXT:    ret void
;
  %1 = call fast float @llvm.genx.GenISA.WaveShuffleIndex.f32(float 0.000000e+00, i32 %lane_a, i32 0)
  %2 = xor i32 %lane_b, 16
  %3 = call fast float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %1, i32 %2, i32 0)
  %4 = fadd fast float %3, %1
  call void @use.f32(float %4)
  ret void
}

; --- Negative case: non-constant source should NOT fold ---

define void @test_waveshuffle_nonconstant_no_fold(i32 %x, i32 %lane) {
; CHECK-LABEL: @test_waveshuffle_nonconstant_no_fold(
; CHECK:         %1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %lane, i32 0)
; CHECK-NEXT:    call void @use.i32(i32 %1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %lane, i32 0)
  call void @use.i32(i32 %1)
  ret void
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare float @llvm.genx.GenISA.WaveShuffleIndex.f32(float, i32, i32)
declare void @use.i32(i32)
declare void @use.f32(float)
