;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Opaque-pointer variant of vec4-to-scalar-insert.ll. The element is written
; back through an insertelement before being stored, exercising the
; InsertElementInst passthrough in GetUsedVectorElements and the store-data
; repack (RepackToNewType) on the single-element path.

; RUN: igc_opt --opaque-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, float %data0) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  %addr = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  %vec0 = load <4 x float>, ptr %addr
; CHECK:        [[Z:%[A-z0-9]*]] = load float, ptr [[ADDR]], align 4
; CHECK:        [[VEC0:%[A-z0-9]*]] = insertelement <4 x float> undef, float [[Z]], i64 2
  %data1 = insertelement <4 x float> %vec0, float %data0, i64 2
; CHECK:        [[VEC1:%[A-z0-9]*]] = insertelement <4 x float> [[VEC0]], float %data0, i64 2
  store <4 x float> %data1, ptr %addr, align 16
; CHECK:        [[ST:%[A-z0-9]*]] = extractelement <4 x float> [[VEC1]], i64 2
; CHECK-NEXT:   store float [[ST]], ptr [[ADDR]], align 4
  %addr1 = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR1:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  %vec1 = load <4 x float>, ptr %addr1
; CHECK:        [[A:%[A-z0-9]*]] = load float, ptr [[ADDR1]], align 4
  %a = extractelement <4 x float> %vec1, i64 2
  ret float %a
; CHECK:        ret float [[A]]
}

attributes #1 = { "null-pointer-is-valid"="true" }
