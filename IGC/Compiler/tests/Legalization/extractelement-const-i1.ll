;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: extractelement from constant bool (i1) vector
; ------------------------------------------------

; IGC cannot extract i1 directly from a constant bool vector
; Legalization widens the vector to i8, extracts and compares to 0.

define i1 @test_constant_mask(i32 %index) {
; CHECK-LABEL: define i1 @test_constant_mask(
; CHECK: [[E:%.*]] = extractelement <4 x i8> <i8 1, i8 0, i8 0, i8 0>, i32 %index
; CHECK-NEXT: [[B:%.*]] = icmp ne i8 [[E]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <4 x i1> <i1 true, i1 false, i1 false, i1 false>, i32 %index
  ret i1 %bit
}

!igc.functions = !{!0}

!0 = !{i1 (i32)* @test_constant_mask, !1}
!1 = !{}
