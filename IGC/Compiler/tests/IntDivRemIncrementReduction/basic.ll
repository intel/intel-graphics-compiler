;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -regkey SanitizeDivRemIncrementDivisorIsZero=1 -S < %s | FileCheck %s
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -regkey GuardDivRemIncrementDividendOverflow=1 -S < %s | FileCheck %s --check-prefix=OVF
; ------------------------------------------------
;
; Simple increment reduction, optimization for second udiv/urem pair based on result of first.
; - Test divisor is zero sanitization logic
; - Test nuw flag detection
; Ensure only can only look past Add/Or/Sub for finding true offset
; Do CSE when two DivRemGroup offsets are the same
; - Test nuw flag detection
; Check that DivRemPairs are still matched when udiv/urem are in reverse order or are not back to back
; Simple decrement reduction
; ------------------------------------------------

; CHECK-LABEL: @test_int_divrem_increment_reduction
; OVF-LABEL: @test_int_divrem_increment_reduction
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

; Sanitize logic to avoid UB
; CHECK-NEXT: [[DIVISOR_IS_ZERO:%.*]] = icmp eq i32 %b, 0
; CHECK-NEXT: [[FINAL_NEXT_QUO:%.*]] = select i1 [[DIVISOR_IS_ZERO]], i32 -1, i32 [[NEW_NEXT_QUO]]
; CHECK-NEXT: [[FINAL_NEXT_REM:%.*]] = select i1 [[DIVISOR_IS_ZERO]], i32 -1, i32 [[NEW_NEXT_REM]]

; Check that original udiv/urem is deleted
; CHECK-NOT: udiv i32 %next, %b
; CHECK-NOT: urem i32 %next, %b
; nuw flag not present on increment instruction, did not optimize, so original udiv/urem should still be present
; OVF: udiv i32 %next, %b
; OVF: urem i32 %next, %b
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b

; Check that original uses are replaced
; CHECK: store i32 [[FINAL_NEXT_QUO]], ptr %dest1
; CHECK: store i32 [[FINAL_NEXT_REM]], ptr %dest2
; Unchanged insts
; OVF: store i32 %next_quo, ptr %dest1
; OVF: store i32 %next_rem, ptr %dest2
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}

; We can't assume what the dividend will look like
; CHECK-LABEL: @test_arbitrary_dividend
; OVF-LABEL: @test_arbitrary_dividend
define i32 @test_arbitrary_dividend(float %a, i32 %b) {
; CHECK: [[BC:%.*]] = bitcast float %a to i32
; CHECK-NEXT: [[UDIV:%.*]] = udiv i32 [[BC]], %b
; CHECK-NEXT: [[UREM:%.*]] = urem i32 [[BC]], %b
  %bc = bitcast float %a to i32
  %1 = udiv i32 %bc, %b
  %2 = urem i32 %bc, %b
  ret i32 %2
}

; Perform CSE for indirectly equal DivRemPairs
; CHECK-LABEL: @test_cse
; OVF-LABEL: @test_cse
define void @test_cse(i32 %a, i32 %b, ptr %dest) {
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b

; Next DivRemGroup, DivRemPair optimization, even when GuardDivRemIncrementDividendOverflow=1, because of add nuw i32 %a, 1
; Without flag it should still be optimized, but similar check to first test, so skip CHECK prefix
; OVF: [[PREINC_REM:%.*]] = add i32 %rem, 1
; OVF-NEXT: [[PREINC_REM_TEST:%.*]] = icmp eq i32 [[PREINC_REM]], %b
; OVF-NEXT: [[PREINC_QUO:%.*]] = add i32 %quo, 1
; OVF-NEXT: [[NEW_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %quo
; OVF-NEXT: [[NEW_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 0, i32 [[PREINC_REM]]
  %next = add nuw i32 %a, 1
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b

; sub has no nuw flag, so do not do replacement/CSE
; OVF: udiv i32 %next2, %b
; OVF: urem i32 %next2, %b
  %next2 = sub i32 %next, 1
  %next2_quo = udiv i32 %next2, %b
  %next2_rem = urem i32 %next2, %b

; Check that uses are replaced
; OVF: %sum_quo = add i32 %quo, [[NEW_NEXT_QUO]]
  %sum_quo = add i32 %quo, %next_quo
; Check that %next2_quo and %next2_rem are CSE'ed to %quo and %rem, because %a + 1 - 1 = %a
; CHECK: %sum_quo2 = add i32 %sum_quo, %quo
; OVF: %sum_quo2 = add i32 %sum_quo, %next2_quo
  %sum_quo2 = add i32 %sum_quo, %next2_quo
; OVF: %sum_rem = add i32 %rem, [[NEW_NEXT_REM]]
  %sum_rem = add i32 %rem, %next_rem
; CHECK: %sum_rem2 = add i32 %sum_rem, %rem
; OVF: %sum_rem2 = add i32 %sum_rem, %next2_rem
  %sum_rem2 = add i32 %sum_rem, %next2_rem
  %res = add i32 %sum_quo2, %sum_rem2
  store i32 %res, ptr %dest
  ret void
}

; reverse order and non-back-to-back udiv/urem, still do optimization
; CHECK-LABEL: @test_reverse_order_and_non_back_to_back
define void @test_reverse_order_and_non_back_to_back(i32 %a, i32 %b, ptr %dest) {
  %rem = urem i32 %a, %b
  %dest_i = ptrtoint ptr %dest to i32
  %dest1_i = add i32 %dest_i, %a
  %quo = udiv i32 %a, %b
  %next = add nuw i32 %a, 1

; Next DivRemGroup, DivRemPair optimization
; CHECK: [[PREINC_REM:%.*]] = add i32 %rem, 1
; CHECK-NEXT: [[PREINC_REM_TEST:%.*]] = icmp eq i32 [[PREINC_REM]], %b
; CHECK-NEXT: [[PREINC_QUO:%.*]] = add i32 %quo, 1
; CHECK-NEXT: [[NEW_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %quo
; CHECK-NEXT: [[NEW_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 0, i32 [[PREINC_REM]]

; Sanitize logic to avoid UB, enabled by default
; CHECK-NEXT: [[DIVISOR_IS_ZERO:%.*]] = icmp eq i32 %b, 0
; CHECK-NEXT: [[FINAL_NEXT_QUO:%.*]] = select i1 [[DIVISOR_IS_ZERO]], i32 -1, i32 [[NEW_NEXT_QUO]]
; CHECK-NEXT: [[FINAL_NEXT_REM:%.*]] = select i1 [[DIVISOR_IS_ZERO]], i32 -1, i32 [[NEW_NEXT_REM]]

; Check that original udiv/urem is deleted
; CHECK-NOT: urem i32 %next, %b
; CHECK-NOT: udiv i32 %next, %b
  %next_rem = urem i32 %next, %b
  %next_quo = udiv i32 %next, %b
  %dest2_i = add i32 %dest_i, %b
  %dest1 = inttoptr i32 %dest1_i to ptr
  %dest2 = inttoptr i32 %dest2_i to ptr
; CHECK: store i32 [[FINAL_NEXT_QUO]], ptr %dest1
; CHECK: store i32 [[FINAL_NEXT_REM]], ptr %dest2
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}

; CHECK-LABEL: @test_int_divrem_decrement_reduction
define void @test_int_divrem_decrement_reduction(i32 %a, i32 %b, ptr %dest1, ptr %dest2) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %next = sub i32 %a, 1

; Next DivRemGroup, TODO: Negative offset DivRemPair optimization
; Ensure unoptimized result emitted for now
; CHECK: udiv i32 %next, %b
; CHECK: urem i32 %next, %b
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}
