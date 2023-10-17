;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Test, based on laplace cm-test
; CHECK-LABEL: @laplace_genx
define spir_kernel void @laplace_genx(<4 x float> %0, <144 x float> %1, <24 x float> %2) {
.preheader764:
; Reduced all uitofp and fdiv
; CHECK-NOT: uitofp
; CHECK-NOT: fdiv
; CHECK: fmul <144 x float> {{.*}}, <
; CHECK-COUNT-144: float 0x3F70101020000000,
; CHECK: fmul <4 x float> {{.*}}, <
; CHECK-COUNT-4: float 0x3F70101020000000,
; CHECK: fmul <24 x float> {{.*}}, <
; CHECK-COUNT-24: float 0x3F70101020000000,
  %3 = fdiv <144 x float> %1, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %4 = fdiv <4 x float> %0, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %5 = fdiv <4 x float> %0, zeroinitializer
  %6 = fdiv <24 x float> %2, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %7 = fdiv <24 x float> %2, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %8 = fdiv <24 x float> %2, <float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02, float 2.550000e+02>
  %.regioncollapsed1042 = tail call <1 x float> @llvm.genx.rdregionf.v1f32.v24f32.i16(<24 x float> %7, i32 0, i32 0, i32 0, i16 0, i32 0)
  %9 = tail call <1 x float> @llvm.genx.rdregionf.v1f32.v24f32.i16(<24 x float> %8, i32 0, i32 0, i32 0, i16 0, i32 0)
  %.regioncollapsed1039 = tail call <1 x float> @llvm.genx.rdregionf.v1f32.v144f32.i16(<144 x float> %3, i32 0, i32 0, i32 0, i16 0, i32 0)
  %.regioncollapsed1033 = tail call <1 x float> @llvm.genx.rdregionf.v1f32.v24f32.i16(<24 x float> %6, i32 0, i32 0, i32 0, i16 0, i32 0)
  %10 = tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> %5, <1 x float> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %11 = tail call <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float> %4, <1 x float> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  ret void
}

declare <4 x float> @llvm.genx.wrregionf.v4f32.v1f32.i16.i1(<4 x float>, <1 x float>, i32, i32, i32, i16, i32, i1)

declare <1 x float> @llvm.genx.rdregionf.v1f32.v24f32.i16(<24 x float>, i32, i32, i32, i16, i32)

declare <1 x float> @llvm.genx.rdregionf.v1f32.v144f32.i16(<144 x float>, i32, i32, i32, i16, i32)
