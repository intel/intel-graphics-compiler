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

define float @f0(i32 %index, i1 %cond, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  br i1 %cond, label %Label-1, label %Label-2

Label-1:
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x float>, <4 x float>* %addr
; CHECK:        [[A:%[A-z0-9]*]] = load float, float* [[ADDR]], align 4
; CHECK-NEXT:   [[VEC0:%[A-z0-9]*]] = insertelement <4 x float> undef, float [[A]], i64 2
  br label %Label-2

Label-2:
  %vec1 = phi <4x float> [ %data, %Label-0 ], [ %vec, %Label-1 ]
; CHECK: [[PHI:%[A-z0-9]*]] = phi <4 x float> [ %data, %Label-0 ], [ [[VEC0]], %Label-1 ]
  %a = extractelement <4 x float> %vec1, i64 2
; CHECK: [[A:%[A-z0-9]*]] = extractelement <4 x float> [[PHI]], i64 2
  ret float %a
; CHECK:        ret float [[A]]
}

attributes #1 = { "null-pointer-is-valid"="true" }

