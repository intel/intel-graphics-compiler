;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @testalloca1() {
0:
  %1 = alloca[356 x float], align 4
  %2 = getelementptr inbounds [356 x float], [356 x float]* %1, i32 0, i32 352
  store float 0.000000e+00, float* %2, align 4
  %3 = getelementptr inbounds [356 x float], [356 x float]* %1, i32 0, i32 1
  store float 1.000000e+00, float* %3, align 4
  ret void
}
; CHECK-LABEL: define spir_kernel void @testalloca1() {
; CHECK:  %1 = alloca [352 x float]
; CHECK:  [[GEP1:%[a-zA-Z0-9]+]] = getelementptr [352 x float], [352 x float]* %1, i32 0, i32 351
; CHECK:  store float 0.000000e+00, float* [[GEP1]], align 4
; CHECK:  [[GEP2:%[a-zA-Z0-9]+]] = getelementptr [352 x float], [352 x float]* %1, i32 0, i32 0
; CHECK:  store float 1.000000e+00, float* [[GEP2]], align 4
; CHECK:  ret void

define spir_kernel void @testalloca2() {
0:
  %1 = alloca[356 x float], align 4
  ret void
}
; CHECK-LABEL: define spir_kernel void @testalloca2() {
; CHECK:  %1 = alloca [356 x float], align 4
; CHECK-NOT:  %2 = alloca[0 x float]

define spir_kernel void @testalloca3(i32 %index) {
0:
  %1 = alloca[356 x float], align 4
  %2 = getelementptr inbounds [356 x float], [356 x float]* %1, i32 0, i32 %index
  store float 0.000000e+00, float* %2, align 4
  ret void
}
; CHECK-LABEL: define spir_kernel void @testalloca3(i32 %index) {
; CHECK:  %1 = alloca [356 x float], align 4
; CHECK:  %2 = getelementptr inbounds [356 x float], [356 x float]* %1, i32 0, i32 %index
; CHECK:  store float 0.000000e+00, float* %2, align 4
; CHECK:  ret void

