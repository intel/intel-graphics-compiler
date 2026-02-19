;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, float %data0) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  %vec0 = load <4 x float>, <4 x float>* %addr
  %data1 = insertelement <4 x float> %vec0, float %data0, i64 2
  store <4 x float> %data1, <4 x float>* %addr, align 16
  %addr1 = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
  %vec1 = load <4 x float>, <4 x float>* %addr1
  %a = extractelement <4 x float> %vec1, i64 2
  ret float %a
}

; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
; CHECK:        [[Z:%[A-z0-9]*]] = load float, float* [[ADDR]], align 4
; CHECK:        [[VEC0:%[A-z0-9]*]] = insertelement <4 x float> undef, float [[Z]], i64 2
; CHECK:        [[VEC1:%[A-z0-9]*]] = insertelement <4 x float> [[VEC0]], float %data0, i64 2
; CHECK:        [[Z:%[A-z0-9]*]] = extractelement <4 x float> [[VEC1]], i64 2
; CHECK-NEXT:   store float [[Z]], float* [[ADDR]], align 4
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
; CHECK:        [[A:%[A-z0-9]*]] = load float, float* [[ADDR]], align 4
; CHECK:        ret float [[A]]

attributes #1 = { "null-pointer-is-valid"="true" }

