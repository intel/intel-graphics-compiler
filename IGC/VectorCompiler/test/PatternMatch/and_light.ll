;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch \
; RUN:  -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -S < %s

; Test verifies that there will be no asserts here
define spir_kernel void @and() {
  %1 = and <16 x i1> zeroinitializer, zeroinitializer
  %2 = bitcast <16 x i1> %1 to i16
  ret void
}

