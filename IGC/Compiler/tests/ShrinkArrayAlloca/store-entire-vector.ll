;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the the optimiztion is not triggered due entire vector being stored.

; RUN: igc_opt %s -S --shrink-array-alloca | FileCheck %s
;

define void @f0(i32 %index, <4 x float> %data, <4 x float>* %ptr) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:       %alloca = alloca [64 x <4 x float>], align 16
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  store <4 x float> %data, <4 x float>* %addr, align 16
  %addr1 = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  %vec = load <4 x float>, <4 x float>* %addr1
  store <4 x float> %vec, <4 x float>* %ptr, align 16
  ret void
}

attributes #1 = { "null-pointer-is-valid"="true" }

