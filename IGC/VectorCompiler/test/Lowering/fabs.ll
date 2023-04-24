;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare float @llvm.fabs.f32(float) #1
declare <4 x float> @llvm.fabs.v4f32(<4 x float>) #1
declare double @llvm.fabs.f64(double) #1
declare <4 x double> @llvm.fabs.v4f64(<4 x double>) #1
attributes #1 = { nounwind readnone speculatable willreturn }

define spir_func float @scalar1(float %src) {
  %res = tail call float @llvm.fabs.f32(float %src)
  ret float %res
}
; CHECK-LABEL: define spir_func float @scalar1
; CHECK-SAME: (float %[[ARG1:[^ )]+]])
; CHECK: %[[RES1:[^ ]+]] = call float @llvm.genx.absf.f32(float %[[ARG1]])
; CHECK-NEXT: ret float %[[RES1]]

define spir_func double @scalar2(double %src) {
  %res = tail call double @llvm.fabs.f64(double %src)
  ret double %res
}
; CHECK-LABEL: define spir_func double @scalar2
; CHECK-SAME: (double %[[ARG3:[^ )]+]])
; CHECK: %[[RES3:[^ ]+]] = call double @llvm.genx.absf.f64(double %[[ARG3]])
; CHECK-NEXT: ret double %[[RES3]]

define spir_func <4 x float> @vector1(<4 x float> %src) {
  %res = tail call <4 x float> @llvm.fabs.v4f32(<4 x float> %src)
  ret <4 x float> %res
}
; CHECK-LABEL: define spir_func <4 x float> @vector1
; CHECK-SAME: (<4 x float> %[[ARG2:[^ )]+]])
; CHECK: %[[RES2:[^ ]+]] = call <4 x float> @llvm.genx.absf.v4f32(<4 x float> %[[ARG2]])
; CHECK-NEXT: ret <4 x float> %[[RES2]]

define spir_func <4 x double> @vector2(<4 x double> %src) {
  %res = tail call <4 x double> @llvm.fabs.v4f64(<4 x double> %src)
  ret <4 x double> %res
}
; CHECK-LABEL: define spir_func <4 x double> @vector2
; CHECK-SAME: (<4 x double> %[[ARG4:[^ )]+]])
; CHECK: %[[RES4:[^ ]+]] = call <4 x double> @llvm.genx.absf.v4f64(<4 x double> %[[ARG4]])
; CHECK-NEXT: ret <4 x double> %[[RES4]]
