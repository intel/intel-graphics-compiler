;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Opaque-pointer variant of vec4-to-scalar-simple.ll. Only one element of the
; vector is used so the [64 x <4 x float>] alloca is shrunk to [64 x float] and
; the vector store/load become scalar.

; RUN: igc_opt --opaque-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  %addr = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  store <4 x float> %data, ptr %addr, align 16
; CHECK:        [[DATAA:%[A-z0-9]*]] = extractelement <4 x float> %data, i64 2
; CHECK-NEXT:   store float [[DATAA]], ptr [[ADDR]], align 4
  %addr1 = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR1:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x float>, ptr %addr1
; CHECK:        [[A:%[A-z0-9]*]] = load float, ptr [[ADDR1]], align 4
  %a = extractelement <4 x float> %vec, i64 2
  ret float %a
; CHECK:        ret float [[A]]
}

attributes #1 = { "null-pointer-is-valid"="true" }
