;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Opaque-pointer variant of vec4-to-scalar-phi.ll. The loaded vector flows
; through a PHI before the single-element extract, exercising the PHINode
; passthrough in GetUsedVectorElements and the RepackToOldType / insertelement
; rebuild that ReplaceUseWith emits at the predecessor terminator.

; RUN: igc_opt --opaque-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, i1 %cond, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  br i1 %cond, label %Label-1, label %Label-2

Label-1:
  %addr = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x float>, ptr %addr
; CHECK:        [[A:%[A-z0-9]*]] = load float, ptr [[ADDR]], align 4
; CHECK-NEXT:   [[VEC0:%[A-z0-9]*]] = insertelement <4 x float> undef, float [[A]], i64 2
  br label %Label-2

Label-2:
  %vec1 = phi <4 x float> [ %data, %Label-0 ], [ %vec, %Label-1 ]
; CHECK:        [[PHI:%[A-z0-9]*]] = phi <4 x float> [ %data, %Label-0 ], [ [[VEC0]], %Label-1 ]
  %a = extractelement <4 x float> %vec1, i64 2
; CHECK:        [[RES:%[A-z0-9]*]] = extractelement <4 x float> [[PHI]], i64 2
  ret float %a
; CHECK:        ret float [[RES]]
}

attributes #1 = { "null-pointer-is-valid"="true" }
