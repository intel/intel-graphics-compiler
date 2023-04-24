;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXAggregatePseudoLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%vec3_t = type { <16 x float>, <16 x float>, <16 x float> }
%folded_st = type { <16 x float>, { <16 x float>, <16 x float> } }

define dllexport void @simple_select(i1 %sel.cond, <16 x float> %sel.true.a, <16 x float> %sel.true.b, <16 x float> %sel.true.c, <16 x float> %sel.false.a, <16 x float> %sel.false.b, <16 x float> %sel.false.c) {
; CHECK-LABEL: simple_select

  %sel.true.0 = insertvalue %vec3_t undef, <16 x float> %sel.true.a, 0
  %sel.true.1 = insertvalue %vec3_t %sel.true.0, <16 x float> %sel.true.b, 1
  %sel.true = insertvalue %vec3_t %sel.true.1, <16 x float> %sel.true.c, 2
  %sel.false.0 = insertvalue %vec3_t undef, <16 x float> %sel.false.a, 0
  %sel.false.1 = insertvalue %vec3_t %sel.false.0, <16 x float> %sel.false.b, 1
  %sel.false = insertvalue %vec3_t %sel.false.1, <16 x float> %sel.false.c, 2

; COM: operand linearization
; CHECK: %[[SEL_TRUE_A:[^ ]+]] = extractvalue %vec3_t %sel.true, 0
; CHECK: %[[SEL_TRUE_B:[^ ]+]] = extractvalue %vec3_t %sel.true, 1
; CHECK: %[[SEL_TRUE_C:[^ ]+]] = extractvalue %vec3_t %sel.true, 2
; CHECK: %[[SEL_FALSE_A:[^ ]+]] = extractvalue %vec3_t %sel.false, 0
; CHECK: %[[SEL_FALSE_B:[^ ]+]] = extractvalue %vec3_t %sel.false, 1
; CHECK: %[[SEL_FALSE_C:[^ ]+]] = extractvalue %vec3_t %sel.false, 2
  %res = select i1 %sel.cond, %vec3_t %sel.true, %vec3_t %sel.false
; COM: split instruction
; CHECK: %[[SEL_RES_A:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_A]], <16 x float> %[[SEL_FALSE_A]]
; CHECK: %[[SEL_RES_B:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_B]], <16 x float> %[[SEL_FALSE_B]]
; CHECK: %[[SEL_RES_C:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_C]], <16 x float> %[[SEL_FALSE_C]]
; COM: combine the aggregate back
; CHECK: %[[SEL_RES_0:[^ ]+]] = insertvalue %vec3_t undef, <16 x float> %[[SEL_RES_A]], 0
; CHECK: %[[SEL_RES_1:[^ ]+]] = insertvalue %vec3_t %[[SEL_RES_0]], <16 x float> %[[SEL_RES_B]], 1
; CHECK: %res = insertvalue %vec3_t %[[SEL_RES_1]], <16 x float> %[[SEL_RES_C]], 2
  ret void
}

define dllexport void @folded_select(i1 %sel.cond, <16 x float> %sel.true.a, <16 x float> %sel.true.b, <16 x float> %sel.true.c, <16 x float> %sel.false.a, <16 x float> %sel.false.b, <16 x float> %sel.false.c) {
; CHECK-LABEL: folded_select

  %sel.true.0 = insertvalue %folded_st undef, <16 x float> %sel.true.a, 0
  %sel.true.1 = insertvalue %folded_st %sel.true.0, <16 x float> %sel.true.b, 1, 0
  %sel.true = insertvalue %folded_st %sel.true.1, <16 x float> %sel.true.c, 1, 1
  %sel.false.0 = insertvalue %folded_st undef, <16 x float> %sel.false.a, 0
  %sel.false.1 = insertvalue %folded_st %sel.false.0, <16 x float> %sel.false.b, 1, 0
  %sel.false = insertvalue %folded_st %sel.false.1, <16 x float> %sel.false.c, 1, 1

; COM: operand linearization
; CHECK: %[[SEL_TRUE_A:[^ ]+]] = extractvalue %folded_st %sel.true, 0
; CHECK: %[[SEL_TRUE_B:[^ ]+]] = extractvalue %folded_st %sel.true, 1, 0
; CHECK: %[[SEL_TRUE_C:[^ ]+]] = extractvalue %folded_st %sel.true, 1, 1
; CHECK: %[[SEL_FALSE_A:[^ ]+]] = extractvalue %folded_st %sel.false, 0
; CHECK: %[[SEL_FALSE_B:[^ ]+]] = extractvalue %folded_st %sel.false, 1, 0
; CHECK: %[[SEL_FALSE_C:[^ ]+]] = extractvalue %folded_st %sel.false, 1, 1
  %res = select i1 %sel.cond, %folded_st %sel.true, %folded_st %sel.false
; COM: split instruction
; CHECK: %[[SEL_RES_A:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_A]], <16 x float> %[[SEL_FALSE_A]]
; CHECK: %[[SEL_RES_B:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_B]], <16 x float> %[[SEL_FALSE_B]]
; CHECK: %[[SEL_RES_C:[^ ]+]] = select i1 %sel.cond, <16 x float> %[[SEL_TRUE_C]], <16 x float> %[[SEL_FALSE_C]]
; COM: combine the aggregate back
; CHECK: %[[SEL_RES_0:[^ ]+]] = insertvalue %folded_st undef, <16 x float> %[[SEL_RES_A]], 0
; CHECK: %[[SEL_RES_1:[^ ]+]] = insertvalue %folded_st %[[SEL_RES_0]], <16 x float> %[[SEL_RES_B]], 1, 0
; CHECK: %res = insertvalue %folded_st %[[SEL_RES_1]], <16 x float> %[[SEL_RES_C]], 1, 1
  ret void
}

