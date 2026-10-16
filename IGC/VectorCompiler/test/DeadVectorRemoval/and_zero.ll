;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: An operand ANDed with zero cannot affect the result, so it is "dead",
; COM: but the instruction still uses it. It must become undef, not poison:
; COM: (and poison, 0) is poison and would poison the live result.

define <4 x i32> @test_and_zero(<4 x i32> %x) {
; CHECK-LABEL: @test_and_zero(
; CHECK: and <4 x i32> zeroinitializer, undef
  %r = and <4 x i32> zeroinitializer, %x
  ret <4 x i32> %r
}
