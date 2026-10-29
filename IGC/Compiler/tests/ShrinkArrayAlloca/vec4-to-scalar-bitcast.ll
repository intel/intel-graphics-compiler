;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Opaque-pointer variant of vec4-to-scalar-bitcast-typed.ll. The vector is
; loaded as <4 x i32> and bitcast to <4 x float> before a single element is
; used, so the [64 x <4 x float>] alloca is shrunk to [64 x float] and the
; load becomes a scalar float load.

; RUN: igc_opt --opaque-pointers %s -S --shrink-array-alloca | FileCheck %s
;

define float @f0(i32 %index) #1 {
Label-0:
  %alloca = alloca [64 x <4 x float>], align 16
; CHECK:        [[ALLOCA:%[A-z0-9]*]] = alloca [64 x float], align 4
  %addr = getelementptr [64 x <4 x float>], ptr %alloca, i32 0, i32 %index
; CHECK:        [[ADDR:%[A-z0-9]*]] = getelementptr [64 x float], ptr [[ALLOCA]], i32 0, i32 %index
  %vec = load <4 x i32>, ptr %addr
; CHECK:        [[A:%[A-z0-9]*]] = load float, ptr [[ADDR]], align 4
  %vec1 = bitcast <4 x i32> %vec to <4 x float>
  %a = extractelement <4 x float> %vec1, i64 2
  ret float %a
; CHECK:        ret float [[A]]
}

attributes #1 = { "null-pointer-is-valid"="true" }
