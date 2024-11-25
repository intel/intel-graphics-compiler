;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-sub-group-func-resolution -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define i1 @test_clustered_broadcast_i1(i1 %src) {
; CHECK-LABEL: @test_clustered_broadcast_i1(
; CHECK:    [[TMP1:%.*]] = call i1 @llvm.genx.GenISA.WaveClusteredBroadcast.i1(i1 %src, i32 8, i32 5, i32 0)
; CHECK:    ret i1 [[TMP1]]
;
  %1 = call spir_func i1 @__builtin_IB_simd_clustered_broadcast_b(i1 %src, i32 8, i32 5)
  ret i1 %1
}

define i8 @test_clustered_broadcast_i8(i8 %src) {
; CHECK-LABEL: @test_clustered_broadcast_i8(
; CHECK:    [[TMP1:%.*]] = call i8 @llvm.genx.GenISA.WaveClusteredBroadcast.i8(i8 %src, i32 8, i32 5, i32 0)
; CHECK:    ret i8 [[TMP1]]
;
  %1 = call spir_func i8 @__builtin_IB_simd_clustered_broadcast_c(i8 %src, i32 8, i32 5)
  ret i8 %1
}

define i16 @test_clustered_broadcast_i16(i16 %src) {
; CHECK-LABEL: @test_clustered_broadcast_i16(
; CHECK:    [[TMP1:%.*]] = call i16 @llvm.genx.GenISA.WaveClusteredBroadcast.i16(i16 %src, i32 8, i32 5, i32 0)
; CHECK:    ret i16 [[TMP1]]
;
  %1 = call spir_func i16 @__builtin_IB_simd_clustered_broadcast_us(i16 %src, i32 8, i32 5)
  ret i16 %1
}

define i32 @test_clustered_broadcast_i32(i32 %src) {
; CHECK-LABEL: @test_clustered_broadcast_i32(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.WaveClusteredBroadcast.i32(i32 %src, i32 8, i32 5, i32 0)
; CHECK:    ret i32 [[TMP1]]
;
  %1 = call spir_func i32 @__builtin_IB_simd_clustered_broadcast(i32 %src, i32 8, i32 5)
  ret i32 %1
}

define float @test_clustered_broadcast_float(float %src) {
; CHECK-LABEL: @test_clustered_broadcast_float(
; CHECK:    [[TMP1:%.*]] = call float @llvm.genx.GenISA.WaveClusteredBroadcast.f32(float %src, i32 8, i32 5, i32 0)
; CHECK:    ret float [[TMP1]]
;
  %1 = call spir_func float @__builtin_IB_simd_clustered_broadcast_f(float %src, i32 8, i32 5)
  ret float %1
}

define half @test_clustered_broadcast_half(half %src) {
; CHECK-LABEL: @test_clustered_broadcast_half(
; CHECK:    [[TMP1:%.*]] = call half @llvm.genx.GenISA.WaveClusteredBroadcast.f16(half %src, i32 8, i32 5, i32 0)
; CHECK:    ret half [[TMP1]]
;
  %1 = call spir_func half @__builtin_IB_simd_clustered_broadcast_h(half %src, i32 8, i32 5)
  ret half %1
}

define double @test_clustered_broadcast_double(double %src) {
; CHECK-LABEL: @test_clustered_broadcast_double(
; CHECK:    [[TMP1:%.*]] = call double @llvm.genx.GenISA.WaveClusteredBroadcast.f64(double %src, i32 8, i32 5, i32 0)
; CHECK:    ret double [[TMP1]]
;
  %1 = call spir_func double @__builtin_IB_simd_clustered_broadcast_df(double %src, i32 8, i32 5)
  ret double %1
}

define float @test_clustered_broadcast_invalid_cluster_size(float %src, i32 %arg) {
; CHECK:    error: cluster_size argument in clustered_broadcast must be constant.
; CHECK:    in function: 'test_clustered_broadcast_invalid_cluster_size'
  %1 = call spir_func float @__builtin_IB_simd_clustered_broadcast_f(float %src, i32 %arg, i32 5)
  ret float %1
}

define float @test_clustered_broadcast_invalid_cluster_lane(float %src, i32 %arg) {
; CHECK:    error: in_cluster_lane argument in clustered_broadcast must be constant.
; CHECK:    in function: 'test_clustered_broadcast_invalid_cluster_lane'
  %1 = call spir_func float @__builtin_IB_simd_clustered_broadcast_f(float %src, i32 8, i32 %arg)
  ret float %1
}

declare spir_func i1 @__builtin_IB_simd_clustered_broadcast_b(i1, i32, i32)
declare spir_func i8 @__builtin_IB_simd_clustered_broadcast_c(i8, i32, i32)
declare spir_func i16 @__builtin_IB_simd_clustered_broadcast_us(i16, i32, i32)
declare spir_func i32 @__builtin_IB_simd_clustered_broadcast(i32, i32, i32)
declare spir_func float @__builtin_IB_simd_clustered_broadcast_f(float, i32, i32)
declare spir_func half @__builtin_IB_simd_clustered_broadcast_h(half, i32, i32)
declare spir_func double @__builtin_IB_simd_clustered_broadcast_df(double, i32, i32)
