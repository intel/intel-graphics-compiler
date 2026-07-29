;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -S < %s | FileCheck %s
; ------------------------------------------------
;
; getRemForDiv anchors on the udiv's operand 0 (the dividend). When that operand is a
; ConstantData (here the literal 4096), its use list isn't maintained on LLVM 22+, so
; iterating its users is illegal. The pass must bail out for such dividends instead of
; crashing; no reduction is performed and the div/rem pair is left untouched.
; ------------------------------------------------

; CHECK-LABEL: @test_const_dividend(
; CHECK: udiv i32 4096, %b
; CHECK: urem i32 4096, %b
; CHECK: ret void
define void @test_const_dividend(i32 %b, ptr %dest1, ptr %dest2) {
  %quo = udiv i32 4096, %b
  %rem = urem i32 4096, %b
  store i32 %quo, ptr %dest1
  store i32 %rem, ptr %dest2
  ret void
}
