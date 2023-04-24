;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXAggregatePseudoLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%vec2_t = type { <16 x float>, <16 x float> }
%folded_st = type { <16 x float>, { <16 x float>, <16 x float> } }

define dllexport void @simple_phi(i1 %phi.cond, <16 x float> %phi.true.a, <16 x float> %phi.true.b, <16 x float> %phi.false.a, <16 x float> %phi.false.b) {
; CHECK-LABEL: simple_phi

entry:
  %phi.true.0 = insertvalue %vec2_t undef, <16 x float> %phi.true.a, 0
  %phi.true = insertvalue %vec2_t %phi.true.0, <16 x float> %phi.true.b, 1
  %phi.false.0 = insertvalue %vec2_t undef, <16 x float> %phi.false.a, 0
  %phi.false = insertvalue %vec2_t %phi.false.0, <16 x float> %phi.false.b, 1
; COM: operand linearization
; CHECK: %[[PHI_TRUE_A:[^ ]+]] = extractvalue %vec2_t %phi.true, 0
; CHECK: %[[PHI_TRUE_B:[^ ]+]] = extractvalue %vec2_t %phi.true, 1
; CHECK: %[[PHI_FALSE_A:[^ ]+]] = extractvalue %vec2_t %phi.false, 0
; CHECK: %[[PHI_FALSE_B:[^ ]+]] = extractvalue %vec2_t %phi.false, 1
  br i1 %phi.cond, label %true, label %false

true:
  br label %false

false:
  %res = phi %vec2_t [ %phi.true, %true ], [ %phi.false, %entry ]
; COM: split instruction
; CHECK: %[[PHI_RES_A:[^ ]+]] = phi <16 x float> [ %[[PHI_TRUE_A]], %true ], [ %[[PHI_FALSE_A]], %entry ]
; CHECK: %[[PHI_RES_B:[^ ]+]] = phi <16 x float> [ %[[PHI_TRUE_B]], %true ], [ %[[PHI_FALSE_B]], %entry ]
; COM: combine the aggregate back
; CHECK: %[[PHI_RES_0:[^ ]+]] = insertvalue %vec2_t undef, <16 x float> %[[PHI_RES_A]], 0
; CHECK: %res = insertvalue %vec2_t %[[PHI_RES_0]], <16 x float> %[[PHI_RES_B]], 1
  ret void
}

define dllexport void @folded_phi(i1 %phi.cond, <16 x float> %phi.true.a, <16 x float> %phi.true.b, <16 x float> %phi.true.c, <16 x float> %phi.false.a, <16 x float> %phi.false.b, <16 x float> %phi.false.c) {
; CHECK-LABEL: folded_phi

entry:
  %phi.true.0 = insertvalue %folded_st undef, <16 x float> %phi.true.a, 0
  %phi.true.1 = insertvalue %folded_st %phi.true.0, <16 x float> %phi.true.b, 1, 0
  %phi.true = insertvalue %folded_st %phi.true.1, <16 x float> %phi.true.c, 1, 1
  %phi.false.0 = insertvalue %folded_st undef, <16 x float> %phi.false.a, 0
  %phi.false.1 = insertvalue %folded_st %phi.false.0, <16 x float> %phi.false.b, 1, 0
  %phi.false = insertvalue %folded_st %phi.false.1, <16 x float> %phi.false.c, 1, 1
; COM: operand linearization
; CHECK: %[[PHI_TRUE_A:[^ ]+]] = extractvalue %folded_st %phi.true, 0
; CHECK: %[[PHI_TRUE_B:[^ ]+]] = extractvalue %folded_st %phi.true, 1, 0
; CHECK: %[[PHI_TRUE_C:[^ ]+]] = extractvalue %folded_st %phi.true, 1, 1
; CHECK: %[[PHI_FALSE_A:[^ ]+]] = extractvalue %folded_st %phi.false, 0
; CHECK: %[[PHI_FALSE_B:[^ ]+]] = extractvalue %folded_st %phi.false, 1, 0
; CHECK: %[[PHI_FALSE_C:[^ ]+]] = extractvalue %folded_st %phi.false, 1, 1
  br i1 %phi.cond, label %true, label %false

true:
  br label %false

false:
  %res = phi %folded_st [ %phi.true, %true ], [ %phi.false, %entry ]
; COM: split instruction
; CHECK: %[[PHI_RES_A:[^ ]+]] = phi <16 x float> [ %[[PHI_TRUE_A]], %true ], [ %[[PHI_FALSE_A]], %entry ]
; CHECK: %[[PHI_RES_B:[^ ]+]] = phi <16 x float> [ %[[PHI_TRUE_B]], %true ], [ %[[PHI_FALSE_B]], %entry ]
; CHECK: %[[PHI_RES_C:[^ ]+]] = phi <16 x float> [ %[[PHI_TRUE_C]], %true ], [ %[[PHI_FALSE_C]], %entry ]
; COM: combine the aggregate back
; CHECK: %[[PHI_RES_0:[^ ]+]] = insertvalue %folded_st undef, <16 x float> %[[PHI_RES_A]], 0
; CHECK: %[[PHI_RES_1:[^ ]+]] = insertvalue %folded_st %[[PHI_RES_0]], <16 x float> %[[PHI_RES_B]], 1, 0
; CHECK: %res = insertvalue %folded_st %[[PHI_RES_1]], <16 x float> %[[PHI_RES_C]], 1, 1
  ret void
}

