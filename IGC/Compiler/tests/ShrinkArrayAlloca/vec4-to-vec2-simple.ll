;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Opaque-pointer variant of vec4-to-vec2-simple.ll. Two elements (1 and 3) of
; the vector are used so the [64 x <4 x float>] alloca is shrunk to
; [64 x <2 x float>], exercising the multi-element repack paths
; (RepackToNewType / RepackToOldType and the index remapping).

; RUN: igc_opt --opaque-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index, <4 x float> %data) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x <2 x float>], align 8
  %addr = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x <2 x float>], ptr [[ALLOCA]], i32 0, i32 %index
  store <4 x float> %data, ptr %addr, align 16
; CHECK:        [[DATAY:%[A-z0-9]*]] = extractelement <4 x float> %data, i64 1
; CHECK-NEXT:   [[DATAZ:%[A-z0-9]*]] = extractelement <4 x float> %data, i64 3
; CHECK-NEXT:   [[VEC0:%[A-z0-9]*]] = insertelement <2 x float> undef, float [[DATAY]], i64 0
; CHECK-NEXT:   [[VEC1:%[A-z0-9]*]] = insertelement <2 x float> [[VEC0]], float [[DATAZ]], i64 1
; CHECK-NEXT:   store <2 x float> [[VEC1]], ptr [[ADDR]], align 8
  %addr1 = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR1:%[A-z0-9]*]] = getelementptr [64 x <2 x float>], ptr [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x float>, ptr %addr1
; CHECK:        [[VEC2:%[A-z0-9]*]] = load <2 x float>, ptr [[ADDR1]], align 8
  %a = extractelement <4 x float> %vec, i64 1
; CHECK:        [[A:%[A-z0-9]*]] = extractelement <2 x float> [[VEC2]], i64 0
  %b = extractelement <4 x float> %vec, i64 3
; CHECK:        [[B:%[A-z0-9]*]] = extractelement <2 x float> [[VEC2]], i64 1
  %c = fadd float %a, %b
; CHECK:        [[C:%[A-z0-9]*]] = fadd float [[A]], [[B]]
  ret float %c
; CHECK:        ret float [[C]]
}

attributes #1 = { "null-pointer-is-valid"="true" }
