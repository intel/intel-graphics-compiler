;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i32 @llvm.abs.i32(i32, i1) #1
declare <4 x i32> @llvm.abs.v4i32(<4 x i32>, i1) #1
declare i64 @llvm.abs.i64(i64, i1) #1
declare <4 x i64> @llvm.abs.v4i64(<4 x i64>, i1) #1
attributes #1 = { nounwind readnone speculatable willreturn }

define spir_func i32 @scalar1(i32 %src) {
  %res = tail call i32 @llvm.abs.i32(i32 %src, i1 false)
  ret i32 %res
}
; CHECK-LABEL: define spir_func i32 @scalar1
; CHECK-SAME: (i32 %[[ARG_i32:[^ )]+]])
; CHECK: %[[RES_i32:[^ ]+]] = call i32 @llvm.genx.absi.i32(i32 %[[ARG_i32]])
; CHECK-NEXT: ret i32 %[[RES_i32]]

define spir_func i64 @scalar2(i64 %src) {
  %res = tail call i64 @llvm.abs.i64(i64 %src, i1 false)
  ret i64 %res
}
; CHECK-LABEL: define spir_func i64 @scalar2
; CHECK-SAME: (i64 %[[ARG_i64:[^ )]+]])
; CHECK: %[[RES_i64:[^ ]+]] = call i64 @llvm.genx.absi.i64(i64 %[[ARG_i64]])
; CHECK-NEXT: ret i64 %[[RES_i64]]

define spir_func <4 x i32> @vector1(<4 x i32> %src) {
  %res = tail call <4 x i32> @llvm.abs.v4i32(<4 x i32> %src, i1 false)
  ret <4 x i32> %res
}
; CHECK-LABEL: define spir_func <4 x i32> @vector1
; CHECK-SAME: (<4 x i32> %[[ARG_v4i32:[^ )]+]])
; CHECK: %[[RES_v4i32:[^ ]+]] = call <4 x i32> @llvm.genx.absi.v4i32(<4 x i32> %[[ARG_v4i32]])
; CHECK-NEXT: ret <4 x i32> %[[RES_v4i32]]

define spir_func <4 x i64> @vector2(<4 x i64> %src) {
  %res = tail call <4 x i64> @llvm.abs.v4i64(<4 x i64> %src, i1 false)
  ret <4 x i64> %res
}
; CHECK-LABEL: define spir_func <4 x i64> @vector2
; CHECK-SAME: (<4 x i64> %[[ARG_v4i64:[^ )]+]])
; CHECK: %[[RES_v4i64:[^ ]+]] = call <4 x i64> @llvm.genx.absi.v4i64(<4 x i64> %[[ARG_v4i64]])
; CHECK-NEXT: ret <4 x i64> %[[RES_v4i64]]
