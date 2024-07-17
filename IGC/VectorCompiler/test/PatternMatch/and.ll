;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch --enable-bfn=true \
; RUN:  -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Test verifies that there will be no asserts here
; CHECK-LABEL: internal spir_func float @and_absf
define internal spir_func float @and_absf(float %0) {
; CHECK: call float @llvm.genx.absf.f32
  %.cast = bitcast float 1.000000e+00 to i32
  %2 = and i32 %.cast, 2147483647
  %.cast22 = bitcast i32 %2 to float
  ret float %.cast22
}