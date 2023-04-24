;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare float @llvm.ceil.f32(float) #1
declare <4 x float> @llvm.ceil.v4f32(<4 x float>) #1
declare float @llvm.floor.f32(float) #1
declare <4 x float> @llvm.floor.v4f32(<4 x float>) #1
declare float @llvm.trunc.f32(float) #1
declare <4 x float> @llvm.trunc.v4f32(<4 x float>) #1

define spir_func float @ceil_scalar(float %src) {
  %res = tail call float @llvm.ceil.f32(float %src)
  ret float %res
}
; CHECK-LABEL: define spir_func float @ceil_scalar
; CHECK-SAME: (float %[[ARG1:[^ )]+]])
; CHECK: %[[RES1:[^ ]+]] = call float @llvm.genx.rndu.f32(float %[[ARG1]])
; CHECK-NEXT: ret float %[[RES1]]

define spir_func <4 x float> @ceil_vec(<4 x float> %src) {
  %res = tail call <4 x float> @llvm.ceil.v4f32(<4 x float> %src)
  ret <4 x float> %res
}
; CHECK-LABEL: define spir_func <4 x float> @ceil_vec
; CHECK-SAME: (<4 x float> %[[ARG2:[^ )]+]])
; CHECK: %[[RES2:[^ ]+]] = call <4 x float> @llvm.genx.rndu.v4f32(<4 x float> %[[ARG2]])
; CHECK-NEXT: ret <4 x float> %[[RES2]]

define spir_func float @floor_scalar(float %src) {
  %res = tail call float @llvm.floor.f32(float %src)
  ret float %res
}
; CHECK-LABEL: define spir_func float @floor_scalar
; CHECK-SAME: (float %[[ARG3:[^ )]+]])
; CHECK: %[[RES3:[^ ]+]] = call float @llvm.genx.rndd.f32(float %[[ARG3]])
; CHECK-NEXT: ret float %[[RES3]]

define spir_func <4 x float> @floor_vec(<4 x float> %src) {
  %res = tail call <4 x float> @llvm.floor.v4f32(<4 x float> %src)
  ret <4 x float> %res
}
; CHECK-LABEL: define spir_func <4 x float> @floor_vec
; CHECK-SAME: (<4 x float> %[[ARG4:[^ )]+]])
; CHECK: %[[RES4:[^ ]+]] = call <4 x float> @llvm.genx.rndd.v4f32(<4 x float> %[[ARG4]])
; CHECK-NEXT: ret <4 x float> %[[RES4]]

define spir_func float @trunc_scalar(float %src) {
  %res = tail call float @llvm.trunc.f32(float %src)
  ret float %res
}
; CHECK-LABEL: define spir_func float @trunc_scalar
; CHECK-SAME: (float %[[ARG5:[^ )]+]])
; CHECK: %[[RES5:[^ ]+]] = call float @llvm.genx.rndz.f32(float %[[ARG5]])
; CHECK-NEXT: ret float %[[RES5]]

define spir_func <4 x float> @trunc_vec(<4 x float> %src) {
  %res = tail call <4 x float> @llvm.trunc.v4f32(<4 x float> %src)
  ret <4 x float> %res
}
; CHECK-LABEL: define spir_func <4 x float> @trunc_vec
; CHECK-SAME: (<4 x float> %[[ARG6:[^ )]+]])
; CHECK: %[[RES6:[^ ]+]] = call <4 x float> @llvm.genx.rndz.v4f32(<4 x float> %[[ARG6]])
; CHECK-NEXT: ret <4 x float> %[[RES6]]

attributes #1 = { nounwind readnone speculatable willreturn }
