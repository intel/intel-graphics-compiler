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
; RUN: -mcpu=Xe3P -S < %s 2>&1 | FileCheck --check-prefix=CHECK-XE3P %s
; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Xe3PLPG -S < %s 2>&1 | FileCheck --check-prefix=CHECK-XE3P %s


; CHECK-NOT: WARNING
; CHECK-XE3P-NOT: WARNING

; CHECK: @test_fadd_kernel
; CHECK: = call <16 x float> @__vc_builtin_atomic_slm_v16f32_v2i8
; CHECK: ret void
; CHECK: @test_fcas_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK: ret void
; CHECK: @test_store_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK: ret void
; CHECK: @test_load_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK: ret void

; CHECK-XE3P: @test_fadd_kernel
; CHECK-XE3P: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK-XE3P: ret void
; CHECK-XE3P: @test_fcas_kernel
; CHECK-XE3P: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK-XE3P: ret void
; CHECK-XE3P: @test_store_kernel
; CHECK-XE3P: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK-XE3P: ret void
; CHECK-XE3P: @test_load_kernel
; CHECK-XE3P: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32
; CHECK-XE3P: ret void


declare <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <16 x float>, <16 x float>, <16 x float>)
declare float @llvm.vc.internal.lsc.atomic.slm.f32.i1.v2i8.i32(i1, i8, i8, i8, <2 x i8>, i32, i32, i16, i32, float, float, float)

define dllexport spir_kernel void @test_fadd_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 19, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_fcas_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 23, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_store_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 11, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_load_kernel(<16 x i1> %pred, <16 x i32> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.slm.v16f32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 10, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_fadd_scalar(i1 %pred, i32 %index, float %src1, float %src2, float %passthru) {
; CHECK-LABEL: @test_fadd_scalar
; CHECK: [[CAST:%[^ ]+]] = bitcast i1 %pred to <1 x i1>
; CHECK: [[ZEXT:%[^ ]+]] = zext <1 x i1> [[CAST]] to <1 x i8>
; CHECK: [[VINDEX:%[^ ]+]] = bitcast i32 %index to <1 x i32>
; CHECK: [[VSRC1:%[^ ]+]] = bitcast float %src1 to <1 x float>
; CHECK: [[VSRC2:%[^ ]+]] = bitcast float %src2 to <1 x float>
; CHECK: [[VPASSTHRU:%[^ ]+]] = bitcast float %passthru to <1 x float>
; CHECK: [[CALL:%[^ ]+]] = call <1 x float> @__vc_builtin_atomic_slm_v1f32_v2i8(<1 x i8> [[ZEXT]], i8 19, <2 x i8> zeroinitializer, i32 0, <1 x i32> [[VINDEX]], i16 1, i32 0, <1 x float> [[VSRC1]], <1 x float> [[VSRC2]], <1 x float> [[VPASSTHRU]])
; CHECK: [[BCAST:%[^ ]+]] = bitcast <1 x float> [[CALL]] to float

; CHECK-XE3P-LABEL: @test_fadd_scalar
; CHECK-XE3P: = tail call float @llvm.vc.internal.lsc.atomic.slm.f32.i1.v2i8.i32

  %1 = tail call float @llvm.vc.internal.lsc.atomic.slm.f32.i1.v2i8.i32(i1 %pred, i8 19, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, i32 %index, i16 1, i32 0, float %src1, float %src2, float %passthru)
  ret void
}

define <16 x float> @__vc_builtin_atomic_slm_v16f32_v2i8(<16 x i8> noundef %pred, i8 noundef signext %op, <2 x i8> %cachecontrols, i32 noundef %base, <16 x i32> noundef %index, i16 noundef signext %scale, i32 noundef %offset, <16 x float> noundef %src1, <16 x float> noundef %src2, <16 x float> noundef %passthru) #0 {
  ret <16 x float> zeroinitializer
}

define <1 x float> @__vc_builtin_atomic_slm_v1f32_v2i8(<1 x i8> noundef %pred, i8 noundef signext %op, <2 x i8> %cachecontrol, i32 noundef %base, <1 x i32> noundef %index, i16 noundef signext %scale, i32 noundef %offset, <1 x float> noundef %src1, <1 x float> noundef %src2, <1 x float> noundef %passthru) #0 {
  ret <1 x float> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
