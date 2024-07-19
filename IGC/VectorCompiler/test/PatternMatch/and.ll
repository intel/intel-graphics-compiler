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
define internal spir_func float @and_absf(float %src) {
; CHECK: [[ABS:%[^ ]+]] = call float @llvm.fabs.f32(float %src)
; CHECK: ret float [[ABS]]
  %cast = bitcast float %src to i32
  %and = and i32 %cast, 2147483647
  %res = bitcast i32 %and to float
  ret float %res
}
