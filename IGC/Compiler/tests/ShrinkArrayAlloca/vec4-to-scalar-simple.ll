;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
  store <4 x float> %data, <4 x float>* %addr, align 16
; CHECK:        [[DATAA:%[A-z0-9]*]] = extractelement <4 x float> %data, i64 2
; CHECK-NEXT:   store float [[DATAA]], float* [[ADDR]], align 4
  %addr1 = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x float>, <4 x float>* %addr1
; CHECK:        [[A:%[A-z0-9]*]] = load float, float* [[ADDR]], align 4
  %a = extractelement <4 x float> %vec, i64 2
  ret float %a
; CHECK:        ret float [[A]]
}

attributes #1 = { "null-pointer-is-valid"="true" }

