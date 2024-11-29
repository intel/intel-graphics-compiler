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

define i8 @test_clustered_scan_exclusive_add_i8(i8 %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_i8(
; CHECK:    [[TMP1:%.*]] = call i8 @llvm.genx.GenISA.WaveClusteredPrefix.i8(i8 %src, i8 0, i32 8, i32 0)
; CHECK:    ret i8 [[TMP1]]
;
  %1 = call spir_func i8 @__builtin_IB_sub_group_clustered_scan_IAdd_i8(i8 %src, i32 8)
  ret i8 %1
}

define i16 @test_clustered_scan_exclusive_add_i16(i16 %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_i16(
; CHECK:    [[TMP1:%.*]] = call i16 @llvm.genx.GenISA.WaveClusteredPrefix.i16(i16 %src, i8 0, i32 8, i32 0)
; CHECK:    ret i16 [[TMP1]]
;
  %1 = call spir_func i16 @__builtin_IB_sub_group_clustered_scan_IAdd_i16(i16 %src, i32 8)
  ret i16 %1
}

define i32 @test_clustered_scan_exclusive_add_i32(i32 %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_i32(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.WaveClusteredPrefix.i32(i32 %src, i8 0, i32 8, i32 0)
; CHECK:    ret i32 [[TMP1]]
;
  %1 = call spir_func i32 @__builtin_IB_sub_group_clustered_scan_IAdd_i32(i32 %src, i32 8)
  ret i32 %1
}

define i64 @test_clustered_scan_exclusive_add_i64(i64 %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_i64(
; CHECK:    [[TMP1:%.*]] = call i64 @llvm.genx.GenISA.WaveClusteredPrefix.i64(i64 %src, i8 0, i32 8, i32 0)
; CHECK:    ret i64 [[TMP1]]
;
  %1 = call spir_func i64 @__builtin_IB_sub_group_clustered_scan_IAdd_i64(i64 %src, i32 8)
  ret i64 %1
}

define half @test_clustered_scan_exclusive_add_half(half %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_half(
; CHECK:    [[TMP1:%.*]] = call half @llvm.genx.GenISA.WaveClusteredPrefix.f16(half %src, i8 9, i32 8, i32 0)
; CHECK:    ret half [[TMP1]]
;
  %1 = call spir_func half @__builtin_IB_sub_group_clustered_scan_FAdd_f16(half %src, i32 8)
  ret half %1
}

define float @test_clustered_scan_exclusive_add_float(float %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_float(
; CHECK:    [[TMP1:%.*]] = call float @llvm.genx.GenISA.WaveClusteredPrefix.f32(float %src, i8 9, i32 8, i32 0)
; CHECK:    ret float [[TMP1]]
;
  %1 = call spir_func float @__builtin_IB_sub_group_clustered_scan_FAdd_f32(float %src, i32 8)
  ret float %1
}

define double @test_clustered_scan_exclusive_add_double(double %src) {
; CHECK-LABEL: @test_clustered_scan_exclusive_add_double(
; CHECK:    [[TMP1:%.*]] = call double @llvm.genx.GenISA.WaveClusteredPrefix.f64(double %src, i8 9, i32 8, i32 0)
; CHECK:    ret double [[TMP1]]
;
  %1 = call spir_func double @__builtin_IB_sub_group_clustered_scan_FAdd_f64(double %src, i32 8)
  ret double %1
}

declare spir_func i8 @__builtin_IB_sub_group_clustered_scan_IAdd_i8(i8, i32)
declare spir_func i16 @__builtin_IB_sub_group_clustered_scan_IAdd_i16(i16, i32)
declare spir_func i32 @__builtin_IB_sub_group_clustered_scan_IAdd_i32(i32, i32)
declare spir_func i64 @__builtin_IB_sub_group_clustered_scan_IAdd_i64(i64, i32)
declare spir_func half @__builtin_IB_sub_group_clustered_scan_FAdd_f16(half, i32)
declare spir_func float @__builtin_IB_sub_group_clustered_scan_FAdd_f32(float, i32)
declare spir_func double @__builtin_IB_sub_group_clustered_scan_FAdd_f64(double, i32)
