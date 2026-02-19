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

define float @f0(i32 %index) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  %addr = getelementptr [64 x <4 x float>], [64 x <4 x float>]* %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], [64 x float]* [[ALLOCA]], i32 0, i32 %index
  %addr1 = bitcast <4 x float>* %addr to <4 x i32>*
; CHECK:        [[ADDR1:%[A-z0-9]*]] = bitcast float* [[ADDR]] to i32*
  %vec = load <4 x i32>, <4 x i32>* %addr1
; CHECK:        [[A:%[A-z0-9]*]] = load i32, i32* [[ADDR1]], align 4
  %vec1 = bitcast <4 x i32> %vec to <4 x float>
; CHECK:        [[A1:%[A-z0-9]*]] = bitcast i32 [[A]] to float
  %a = extractelement <4 x float> %vec1, i64 2
  ret float %a
; CHECK:        ret float [[A1]]
}

attributes #1 = { "null-pointer-is-valid"="true" }

