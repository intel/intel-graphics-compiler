;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gen-specific-pattern -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: FNeg to FSub
; ------------------------------------------------

define float @test_fneg(float %src) {
; CHECK-LABEL: @test_fneg(
; CHECK:    [[TMP1:%[0-9]+]] = fsub float 0.000000e+00, %src
; CHECK:    ret float [[TMP1]]

  %1 = fneg float %src
  ret float %1
}