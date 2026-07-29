;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-custom-safe-opt -opaque-pointers -S < %s | FileCheck %s
; ------------------------------------------------
;
; The udiv/urem CSE anchors on operand 0. When that operand is a ConstantData
; (here the literal 4096), its use list isn't maintained on LLVM 22+, so
; iterating its users is illegal. CustomSafeOptPass must bail out for such
; operands instead of crashing; no CSE is performed and both divisions remain.
; ------------------------------------------------

; CHECK-LABEL: @test_udiv_urem_const_dividend(
; CHECK: entry:
; CHECK: udiv i32 4096, %b
; CHECK: urem i32 4096, %b
; CHECK: merge:
; CHECK: udiv i32 4096, %b
; CHECK: urem i32 4096, %b
; CHECK: ret void
define void @test_udiv_urem_const_dividend(i32 %b, i32 %c, i1 %cond, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4) {
entry:
  %udiv1 = udiv i32 4096, %b
  %urem1 = urem i32 4096, %b
  store i32 %udiv1, ptr %dest1
  store i32 %urem1, ptr %dest2
  br i1 %cond, label %path1, label %path2

path1:
  %val1 = add i32 %c, 1
  br label %merge

path2:
  %val2 = add i32 %c, 2
  br label %merge

merge:
  %merged = phi i32 [ %val1, %path1 ], [ %val2, %path2 ]
  %udiv2 = udiv i32 4096, %b
  %urem2 = urem i32 4096, %b
  %res1 = add i32 %merged, %udiv2
  %res2 = add i32 %merged, %urem2
  store i32 %res1, ptr %dest3
  store i32 %res2, ptr %dest4
  ret void
}
