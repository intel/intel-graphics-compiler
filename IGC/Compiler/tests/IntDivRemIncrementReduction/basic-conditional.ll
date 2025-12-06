;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -regkey DivRemIncrementCondBranchSimplify=1 -S < %s | FileCheck %s
; ------------------------------------------------
;
; Conditional reduction, optimization for second udiv/urem pair based on result of first
; ------------------------------------------------

define void @test_int_divrem_increment_reduction_cond(i32 %a, i32 %b, ptr %dest1, ptr %dest2) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %next = add i32 %a, 2
; CHECK: [[NEXT_TEST_OPT:%.*]] = icmp ule i32 2, %b
; CHECK-NEXT: br i1 [[NEXT_TEST_OPT]], label %[[SIMPLE:.*]], label %[[NORMAL:.*]]

; CHECK: [[SIMPLE]]:
; Next DivRemGroup, DivRemPair optimization
; CHECK-NEXT: [[PREINC_REM:%.*]] = add i32 %rem, 2
; CHECK-NEXT: [[PREINC_REM_TEST:%.*]] = icmp uge i32 [[PREINC_REM]], %b
; CHECK-NEXT: [[PREINC_QUO:%.*]] = add i32 %quo, 1
; CHECK-NEXT: [[PREDEC_REM:%.*]] = sub i32 [[PREINC_REM]], %b
; CHECK-NEXT: [[NEW_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %quo
; CHECK-NEXT: [[NEW_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREDEC_REM]], i32 [[PREINC_REM]]
; CHECK: br label %[[END:.*]]

; CHECK: [[NORMAL]]:
; Fallback to normal udiv/urem
; Check that original udiv/urem is still present
; CHECK: [[NORMAL_QUO:%.*]] = udiv i32 %next, %b
; CHECK-NEXT: [[NORMAL_REM:%.*]] = urem i32 %next, %b
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b
; CHECK: br label %[[END]]

; CHECK: [[END]]:
; CHECK: [[JOIN_NEXT_QUO:%.*]] = phi i32 [ [[NORMAL_QUO]], %[[NORMAL]] ], [ [[NEW_NEXT_QUO]], %[[SIMPLE]] ]
; CHECK-NEXT: [[JOIN_NEXT_REM:%.*]] = phi i32 [ [[NORMAL_REM]], %[[NORMAL]] ], [ [[NEW_NEXT_REM]], %[[SIMPLE]] ]

; Check that original uses are replaced
; CHECK-NEXT: store i32 [[JOIN_NEXT_QUO]], ptr %dest1
; CHECK-NEXT: store i32 [[JOIN_NEXT_REM]], ptr %dest2
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  ret void
}
