;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivRemIncrementReduction
;
; Simple reduction with or as increment (instead of add), optimization for second udiv/urem pair based on result of first
; ------------------------------------------------

; CHECK-LABEL: @test_or_one
define void @test_or_one(i32 %a, i32 %b, ptr %dest1, ptr %dest2) {
  %x = shl i32 %a, 1
; First DivRemGroup, retain
  %quo = udiv i32 %x, %b
  %rem = urem i32 %x, %b
  %next = or i32 %x, 1

; Next DivRemGroup, DivRemPair optimization
; CHECK: [[PREINC_REM:%.*]] = add i32 %rem, 1
; CHECK-NEXT: [[PREINC_REM_TEST:%.*]] = icmp eq i32 [[PREINC_REM]], %b
; CHECK-NEXT: [[PREINC_QUO:%.*]] = add i32 %quo, 1
; CHECK-NEXT: [[NEW_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %quo
; CHECK-NEXT: [[NEW_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 0, i32 [[PREINC_REM]]

; Check that original udiv/urem is deleted
; CHECK-NOT: udiv
; CHECK-NOT: urem
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b

; Check that original uses are replaced
; CHECK: store i32 [[NEW_NEXT_QUO]], ptr %dest1
; CHECK: store i32 [[NEW_NEXT_REM]], ptr %dest2
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}

; CHECK-LABEL: @test_or_three
define void @test_or_three(i32 %a, i32 %b, ptr %dest1, ptr %dest2) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %next = or i32 %a, 3

; Next DivRemGroup, DivRemPair optimization not done since common bits set or unknown (in this example) in OR statement
; CHECK: %next_quo = udiv i32 %next, %b
; CHECK: %next_rem = urem i32 %next, %b
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}