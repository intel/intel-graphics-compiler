;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the the optimiztion is not triggered due to the non-constant index in
; the extract element instruction.

; RUN: igc_opt %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:       %alloca = alloca [64 x <4 x float>], align 16
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  store <4 x float> %data, <4 x float>* %addr, align 16
  %addr1 = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  %vec = load <4 x float>, <4 x float>* %addr1
  %a = extractelement <4 x float> %vec, i32 %index
  ret float %a
}

attributes #1 = { "null-pointer-is-valid"="true" }

