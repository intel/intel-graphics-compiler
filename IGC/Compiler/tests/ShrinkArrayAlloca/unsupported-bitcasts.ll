;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the the optimiztion is not triggered due unsupported bitast instructions

; RUN: igc_opt %s -S --shrink-array-alloca | FileCheck %s
;

define i32* @f0(i32 %index) #1 {
Label-0:
; CHECK-LABEL: @f0(
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK: %alloca = alloca [64 x <4 x float>], align 16
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  %addr1 = bitcast <4 x float>* %addr to i32*
  ret i32* %addr1
}

define i32 @f1(i32 %index, <2 x half> %data) #1 {
Label-0:
; CHECK-LABEL: @f1(
  %alloca = alloca [64 x <2 x half>], align 16
; CHECK: %alloca = alloca [64 x <2 x half>], align 16
  %addr = getelementptr [64 x <2 x half>], [64 x <2 x half>]* %alloca, i32 0, i32 %index
  store <2 x half> %data, <2 x half>* %addr, align 16
  %addr1 = getelementptr [64 x <2 x half>], [64 x <2 x half>]* %alloca, i32 0, i32 %index
  %vec = load <2 x half>, <2 x half>* %addr1
  %a = bitcast <2 x half> %vec to i32
  ret i32 %a
}

attributes #1 = { "null-pointer-is-valid"="true" }

