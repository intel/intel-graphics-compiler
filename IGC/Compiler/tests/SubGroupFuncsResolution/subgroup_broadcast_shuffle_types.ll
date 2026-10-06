;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-sub-group-func-resolution -check-debugify -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck %s --check-prefix=NO-I1
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; Checks lowering of __builtin_IB_simd_broadcast* and __builtin_IB_simd_shuffle* for all data types,
; and of __builtin_IB_simd_shuffle_down* for bool and its own overloads.

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; No i1 variant of the intrinsics may be created anywhere in the module.
; NO-I1-NOT: @llvm.genx.GenISA.WaveBroadcast.i1(
; NO-I1-NOT: @llvm.genx.GenISA.WaveShuffleIndex.i1(
; NO-I1-NOT: @llvm.genx.GenISA.simdShuffleDown.i1(

define i1 @test_broadcast_i1(i1 %src) {
; CHECK-LABEL: @test_broadcast_i1(
; CHECK:    [[ZEXT:%.*]] = zext i1 %src to i8
; CHECK:    [[BCAST:%.*]] = call i8 @llvm.genx.GenISA.WaveBroadcast.i8(i8 [[ZEXT]], i32 5, i32 0)
; CHECK:    [[TRUNC:%.*]] = trunc i8 [[BCAST]] to i1
; CHECK:    ret i1 [[TRUNC]]
;
  %1 = call spir_func i1 @__builtin_IB_simd_broadcast_b(i1 %src, i32 5)
  ret i1 %1
}

define i1 @test_broadcast_i1_dynamic_lane(i1 %src, i32 %lane) {
; CHECK-LABEL: @test_broadcast_i1_dynamic_lane(
; CHECK:    [[ZEXT:%.*]] = zext i1 %src to i8
; CHECK:    [[BCAST:%.*]] = call i8 @llvm.genx.GenISA.WaveBroadcast.i8(i8 [[ZEXT]], i32 %lane, i32 0)
; CHECK:    [[TRUNC:%.*]] = trunc i8 [[BCAST]] to i1
; CHECK:    ret i1 [[TRUNC]]
;
  %1 = call spir_func i1 @__builtin_IB_simd_broadcast_b(i1 %src, i32 %lane)
  ret i1 %1
}

define i8 @test_broadcast_i8(i8 %src) {
; CHECK-LABEL: @test_broadcast_i8(
; CHECK:    [[BCAST:%.*]] = call i8 @llvm.genx.GenISA.WaveBroadcast.i8(i8 %src, i32 5, i32 0)
; CHECK:    ret i8 [[BCAST]]
;
  %1 = call spir_func i8 @__builtin_IB_simd_broadcast_c(i8 %src, i32 5)
  ret i8 %1
}

define i16 @test_broadcast_i16(i16 %src) {
; CHECK-LABEL: @test_broadcast_i16(
; CHECK:    [[BCAST:%.*]] = call i16 @llvm.genx.GenISA.WaveBroadcast.i16(i16 %src, i32 5, i32 0)
; CHECK:    ret i16 [[BCAST]]
;
  %1 = call spir_func i16 @__builtin_IB_simd_broadcast_us(i16 %src, i32 5)
  ret i16 %1
}

define i32 @test_broadcast_i32(i32 %src) {
; CHECK-LABEL: @test_broadcast_i32(
; CHECK:    [[BCAST:%.*]] = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %src, i32 5, i32 0)
; CHECK:    ret i32 [[BCAST]]
;
  %1 = call spir_func i32 @__builtin_IB_simd_broadcast(i32 %src, i32 5)
  ret i32 %1
}

define float @test_broadcast_float(float %src) {
; CHECK-LABEL: @test_broadcast_float(
; CHECK:    [[BCAST:%.*]] = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 5, i32 0)
; CHECK:    ret float [[BCAST]]
;
  %1 = call spir_func float @__builtin_IB_simd_broadcast_f(float %src, i32 5)
  ret float %1
}

define half @test_broadcast_half(half %src) {
; CHECK-LABEL: @test_broadcast_half(
; CHECK:    [[BCAST:%.*]] = call half @llvm.genx.GenISA.WaveBroadcast.f16(half %src, i32 5, i32 0)
; CHECK:    ret half [[BCAST]]
;
  %1 = call spir_func half @__builtin_IB_simd_broadcast_h(half %src, i32 5)
  ret half %1
}

define double @test_broadcast_double(double %src) {
; CHECK-LABEL: @test_broadcast_double(
; CHECK:    [[BCAST:%.*]] = call double @llvm.genx.GenISA.WaveBroadcast.f64(double %src, i32 5, i32 0)
; CHECK:    ret double [[BCAST]]
;
  %1 = call spir_func double @__builtin_IB_simd_broadcast_df(double %src, i32 5)
  ret double %1
}

define i1 @test_shuffle_i1(i1 %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_i1(
; CHECK:    [[ZEXT:%.*]] = zext i1 %src to i8
; CHECK:    [[SHUF:%.*]] = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 [[ZEXT]], i32 %lane, i32 0)
; CHECK:    [[TRUNC:%.*]] = trunc i8 [[SHUF]] to i1
; CHECK:    ret i1 [[TRUNC]]
;
  %1 = call spir_func i1 @__builtin_IB_simd_shuffle_b(i1 %src, i32 %lane)
  ret i1 %1
}

define i8 @test_shuffle_i8(i8 %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_i8(
; CHECK:    [[SHUF:%.*]] = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 %src, i32 %lane, i32 0)
; CHECK:    ret i8 [[SHUF]]
;
  %1 = call spir_func i8 @__builtin_IB_simd_shuffle_c(i8 %src, i32 %lane)
  ret i8 %1
}

define i16 @test_shuffle_i16(i16 %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_i16(
; CHECK:    [[SHUF:%.*]] = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 %src, i32 %lane, i32 0)
; CHECK:    ret i16 [[SHUF]]
;
  %1 = call spir_func i16 @__builtin_IB_simd_shuffle_us(i16 %src, i32 %lane)
  ret i16 %1
}

define i32 @test_shuffle_i32(i32 %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_i32(
; CHECK:    [[SHUF:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %src, i32 %lane, i32 0)
; CHECK:    ret i32 [[SHUF]]
;
  %1 = call spir_func i32 @__builtin_IB_simd_shuffle(i32 %src, i32 %lane)
  ret i32 %1
}

define float @test_shuffle_float(float %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_float(
; CHECK:    [[SHUF:%.*]] = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %src, i32 %lane, i32 0)
; CHECK:    ret float [[SHUF]]
;
  %1 = call spir_func float @__builtin_IB_simd_shuffle_f(float %src, i32 %lane)
  ret float %1
}

define half @test_shuffle_half(half %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_half(
; CHECK:    [[SHUF:%.*]] = call half @llvm.genx.GenISA.WaveShuffleIndex.f16(half %src, i32 %lane, i32 0)
; CHECK:    ret half [[SHUF]]
;
  %1 = call spir_func half @__builtin_IB_simd_shuffle_h(half %src, i32 %lane)
  ret half %1
}

define double @test_shuffle_double(double %src, i32 %lane) {
; CHECK-LABEL: @test_shuffle_double(
; CHECK:    [[SHUF:%.*]] = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %src, i32 %lane, i32 0)
; CHECK:    ret double [[SHUF]]
;
  %1 = call spir_func double @__builtin_IB_simd_shuffle_df(double %src, i32 %lane)
  ret double %1
}

; Both data operands (current and next) must be widened.
define i1 @test_shuffle_down_i1(i1 %cur, i1 %next, i32 %delta) {
; CHECK-LABEL: @test_shuffle_down_i1(
; CHECK:    [[ZCUR:%.*]] = zext i1 %cur to i8
; CHECK:    [[ZNEXT:%.*]] = zext i1 %next to i8
; CHECK:    [[SHUF:%.*]] = call i8 @llvm.genx.GenISA.simdShuffleDown.i8(i8 [[ZCUR]], i8 [[ZNEXT]], i32 %delta)
; CHECK:    [[TRUNC:%.*]] = trunc i8 [[SHUF]] to i1
; CHECK:    ret i1 [[TRUNC]]
;
  %1 = call spir_func i1 @__builtin_IB_simd_shuffle_down(i1 %cur, i1 %next, i32 %delta)
  ret i1 %1
}

define i8 @test_shuffle_down_i8(i8 %cur, i8 %next, i32 %delta) {
; CHECK-LABEL: @test_shuffle_down_i8(
; CHECK:    [[SHUF:%.*]] = call i8 @llvm.genx.GenISA.simdShuffleDown.i8(i8 %cur, i8 %next, i32 %delta)
; CHECK:    ret i8 [[SHUF]]
;
  %1 = call spir_func i8 @__builtin_IB_simd_shuffle_down_uc(i8 %cur, i8 %next, i32 %delta)
  ret i8 %1
}

define i16 @test_shuffle_down_i16(i16 %cur, i16 %next, i32 %delta) {
; CHECK-LABEL: @test_shuffle_down_i16(
; CHECK:    [[SHUF:%.*]] = call i16 @llvm.genx.GenISA.simdShuffleDown.i16(i16 %cur, i16 %next, i32 %delta)
; CHECK:    ret i16 [[SHUF]]
;
  %1 = call spir_func i16 @__builtin_IB_simd_shuffle_down_us(i16 %cur, i16 %next, i32 %delta)
  ret i16 %1
}

declare spir_func i1 @__builtin_IB_simd_broadcast_b(i1, i32)
declare spir_func i8 @__builtin_IB_simd_broadcast_c(i8, i32)
declare spir_func i16 @__builtin_IB_simd_broadcast_us(i16, i32)
declare spir_func i32 @__builtin_IB_simd_broadcast(i32, i32)
declare spir_func float @__builtin_IB_simd_broadcast_f(float, i32)
declare spir_func half @__builtin_IB_simd_broadcast_h(half, i32)
declare spir_func double @__builtin_IB_simd_broadcast_df(double, i32)
declare spir_func i1 @__builtin_IB_simd_shuffle_b(i1, i32)
declare spir_func i8 @__builtin_IB_simd_shuffle_c(i8, i32)
declare spir_func i16 @__builtin_IB_simd_shuffle_us(i16, i32)
declare spir_func i32 @__builtin_IB_simd_shuffle(i32, i32)
declare spir_func float @__builtin_IB_simd_shuffle_f(float, i32)
declare spir_func half @__builtin_IB_simd_shuffle_h(half, i32)
declare spir_func double @__builtin_IB_simd_shuffle_df(double, i32)
declare spir_func i1 @__builtin_IB_simd_shuffle_down(i1, i1, i32)
declare spir_func i8 @__builtin_IB_simd_shuffle_down_uc(i8, i8, i32)
declare spir_func i16 @__builtin_IB_simd_shuffle_down_us(i16, i16, i32)
