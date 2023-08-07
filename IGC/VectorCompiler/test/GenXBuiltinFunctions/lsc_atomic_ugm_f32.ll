;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeLPG -S < %s 2>&1 | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeHPC -S < %s 2>&1 | FileCheck --check-prefix=CHECK-2 %s

; CHECK-NOT: WARNING
; CHECK-2-NOT: WARNING

; CHECK: @test_fadd_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64
; CHECK: ret void
; CHECK: @test_fcas_kernel
; CHECK: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64
; CHECK: ret void
; CHECK-2: @test_fadd_kernel
; CHECK-2: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64
; CHECK-2: ret void
; CHECK-2: @test_fcas_kernel
; CHECK-2: = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64
; CHECK-2: ret void

declare <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64(<16 x i1>, i8, i8, i8, i8, i8, i64, <16 x i64>, i16, i32, <16 x float>, <16 x float>, <16 x float>)

define dllexport spir_kernel void @test_fadd_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64(<16 x i1> %pred, i8 19, i8 3, i8 3, i8 0, i8 0, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}

define dllexport spir_kernel void @test_fcas_kernel(<16 x i1> %pred, <16 x i64> %index, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru) {
  %1 = tail call <16 x float> @llvm.vc.internal.lsc.atomic.ugm.v16f32.v16i1.v16i64(<16 x i1> %pred, i8 23, i8 3, i8 3, i8 0, i8 0, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x float> %src1, <16 x float> %src2, <16 x float> %passthru)
  ret void
}
