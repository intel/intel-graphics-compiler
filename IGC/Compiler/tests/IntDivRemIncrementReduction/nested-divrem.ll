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
; Nested reduction, optimization for second udiv/urem group based on result of first
; ------------------------------------------------

define void @test_int_divrem_increment_reduction(i32 %a, i32 %b, i32 %c, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %nested_quo = udiv i32 %quo, %c
  %nested_rem = urem i32 %quo, %c
  %next = add i32 %a, 1

; Next DivRemGroup, first DivRemPair optimization
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

; Next DivRemGroup, second (nested) DivRemPair optimization
; CHECK: [[PREINC_NESTED_REM:%.*]] = add i32 %nested_rem, 1
; CHECK-NEXT: [[PREINC_NESTED_REM_TEST:%.*]] = icmp eq i32 [[PREINC_NESTED_REM]], %c
; CHECK-NEXT: [[PREINC_NESTED_QUO:%.*]] = add i32 %nested_quo, 1
; CHECK-NEXT: [[NESTED_QUO_COND:%.*]] = and i1 [[PREINC_NESTED_REM_TEST]], [[PREINC_REM_TEST]]
; CHECK-NEXT: [[NEW_NEXT_NESTED_QUO:%.*]] = select i1 [[NESTED_QUO_COND]], i32 [[PREINC_NESTED_QUO]], i32 %nested_quo
; CHECK-NEXT: [[NEW_NEXT_NESTED_REM:%.*]] = select i1 [[PREINC_NESTED_REM_TEST]], i32 0, i32 [[PREINC_NESTED_REM]]
; CHECK-NEXT: [[MERGE_NEXT_NESTED_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[NEW_NEXT_NESTED_REM]], i32 %nested_rem

; Check that original next nested udiv/urem is deleted
; CHECK-NOT: udiv
; CHECK-NOT: urem
  %next_nested_quo = udiv i32 %next_quo, %c
  %next_nested_rem = urem i32 %next_quo, %c

; Check that original uses are replaced
; CHECK: store i32 [[NEW_NEXT_QUO]], ptr %dest1
; CHECK: store i32 [[NEW_NEXT_REM]], ptr %dest2
; CHECK: store i32 [[NEW_NEXT_NESTED_QUO]], ptr %dest3
; CHECK: store i32 [[MERGE_NEXT_NESTED_REM]], ptr %dest4
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  store i32 %next_nested_quo, ptr %dest3
  store i32 %next_nested_rem, ptr %dest4
  ret void
}

