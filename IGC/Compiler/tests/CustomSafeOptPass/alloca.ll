;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-custom-safe-opt -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-custom-safe-opt -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-TYPED --input-file=%t.typed.ll %}

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
; CHECK-TYPED:  [[GEP1:%[a-zA-Z0-9]+]] = getelementptr [352 x float], [352 x float]* %1, i32 0, i32 351
; CHECK-OPAQUE:  [[GEP1:%[a-zA-Z0-9]+]] = getelementptr [352 x float], ptr %1, i32 0, i32 351
; CHECK-TYPED:  store float 0.000000e+00, float* [[GEP1]], align 4
; CHECK-OPAQUE:  store float 0.000000e+00, ptr [[GEP1]], align 4
; CHECK-TYPED:  [[GEP2:%[a-zA-Z0-9]+]] = getelementptr [352 x float], [352 x float]* %1, i32 0, i32 0
; CHECK-OPAQUE:  [[GEP2:%[a-zA-Z0-9]+]] = getelementptr [352 x float], ptr %1, i32 0, i32 0
; CHECK-TYPED:  store float 1.000000e+00, float* [[GEP2]], align 4
; CHECK-OPAQUE:  store float 1.000000e+00, ptr [[GEP2]], align 4
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
; CHECK-TYPED:  %2 = getelementptr inbounds [356 x float], [356 x float]* %1, i32 0, i32 %index
; CHECK-OPAQUE:  %2 = getelementptr inbounds [356 x float], ptr %1, i32 0, i32 %index
; CHECK-TYPED:  store float 0.000000e+00, float* %2, align 4
; CHECK-OPAQUE:  store float 0.000000e+00, ptr %2, align 4
; CHECK:  ret void

