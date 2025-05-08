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
; Simple reduction, optimization for second udiv/urem pair based on result of first
; ------------------------------------------------

define void @test_int_divrem_increment_reduction(i32 %a, i32 %b, ptr %dest1, ptr %dest2) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %next = add i32 %a, 1

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

; We can't assume what the dividend will look like
define i32 @test_arbitrary_dividend(float %a, i32 %b) {
; CHECK-LABEL: @test_arbitrary_dividend(
; CHECK: [[BC:%.*]] = bitcast float %a to i32
; CHECK-NEXT: [[UDIV:%.*]] = udiv i32 [[BC]], %b
; CHECK-NEXT: [[UREM:%.*]] = urem i32 [[BC]], %b
  %bc = bitcast float %a to i32
  %1 = udiv i32 %bc, %b
  %2 = urem i32 %bc, %b
  ret i32 %2
}

