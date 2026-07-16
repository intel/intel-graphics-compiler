;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPG -S < %s 2>&1 | FileCheck %s \
; RUN: --implicit-check-not=WARNING --implicit-check-not=@__vc_builtin_atomic_

; CHECK: @test_ugm_fadd_kernel
; CHECK: = tail call <16 x double> @llvm.vc.internal.lsc.atomic.ugm.v16f64.v16i1.v2i8.v16i64
; CHECK: ret void
; CHECK: @test_ugm_fcas_kernel
; CHECK: = tail call <16 x double> @llvm.vc.internal.lsc.atomic.ugm.v16f64.v16i1.v2i8.v16i64
; CHECK: ret void
; CHECK: @test_slm_fadd_kernel
; CHECK: = tail call <16 x double> @llvm.vc.internal.lsc.atomic.slm.v16f64.v16i1.v2i8.v16i32
; CHECK: ret void
; CHECK: @test_slm_fcas_kernel
; CHECK: = tail call <16 x double> @llvm.vc.internal.lsc.atomic.slm.v16f64.v16i1.v2i8.v16i32
; CHECK: ret void

declare <16 x double> @llvm.vc.internal.lsc.atomic.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x double>, <16 x double>, <16 x double>)
declare <16 x double> @llvm.vc.internal.lsc.atomic.slm.v16f64.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <16 x double>, <16 x double>, <16 x double>)

define dllexport spir_kernel void @test_ugm_fadd_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru) {
  %1 = tail call <16 x double> @llvm.vc.internal.lsc.atomic.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 19, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru)
  ret void
}

define dllexport spir_kernel void @test_ugm_fcas_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru) {
  %1 = tail call <16 x double> @llvm.vc.internal.lsc.atomic.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 23, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru)
  ret void
}

define dllexport spir_kernel void @test_slm_fadd_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru) {
  %1 = tail call <16 x double> @llvm.vc.internal.lsc.atomic.slm.v16f64.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 19, i8 2, i8 4, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru)
  ret void
}

define dllexport spir_kernel void @test_slm_fcas_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru) {
  %1 = tail call <16 x double> @llvm.vc.internal.lsc.atomic.slm.v16f64.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 23, i8 2, i8 4, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x double> %src1, <16 x double> %src2, <16 x double> %passthru)
  ret void
}
