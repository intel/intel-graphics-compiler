;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeLPG -S < %s 2>&1 | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -S < %s 2>&1 | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Xe3P -S < %s 2>&1 | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Xe3PLPG -S < %s 2>&1 | FileCheck %s

; CHECK-NOT: WARNING

; CHECK: @test_fadd_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64
; CHECK: ret void
; CHECK: @test_fcas_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64
; CHECK: ret void
; CHECK: @test_store_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64
; CHECK: ret void
; CHECK: @test_load_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64
; CHECK: ret void

declare <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x float>, <16 x float>, <16 x float>)

define dllexport spir_kernel void @test_fadd_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 19, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_fcas_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 23, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_store_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 11, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_load_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 10, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define <16 x float> @__vc_builtin_atomic_ugm_v16f32_v2i8(<16 x i8> noundef %pred, i8 noundef signext %op, <2 x i8> %cachecontrols, i64 noundef %base, <16 x i64> noundef %index, i16 noundef signext %scale, i32 noundef %offset, <16 x float> noundef %src1, <16 x float> noundef %src2, <16 x float> noundef %passthru) #0 {
  ret <16 x float> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
