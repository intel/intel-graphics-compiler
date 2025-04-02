;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -S < %s | FileCheck %s --check-prefix=UNCOND
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -regkey DivRemIncrementCondBranchSimplify=1 -S < %s | FileCheck %s --check-prefix=COND
; ------------------------------------------------
; IntDivRemIncrementReduction
;
; Nested reduction with constant increment > 1, optimize with cond branch vs leave udiv the same
; ------------------------------------------------

define void @test_int_divrem_increment_reduction(i32 %a, i32 %b, i32 %c, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4, ptr %dest5, ptr %dest6, ptr %dest7, ptr %dest8) {
; First DivRemGroup, retain
  %quo = udiv i32 %a, %b
  %rem = urem i32 %a, %b
  %nested_quo = udiv i32 %quo, %c
  %nested_rem = urem i32 %quo, %c
  %next = add i32 %a, 16 ; Add by 16

; Next DivRemGroup optimization

; - DivRemIncrementCondBranchSimplify=0
; conditional branch simplify not allowed, increment by constant greater than 1, do not optimize
; UNCOND: [[NEXT_QUO:%.*]] = udiv i32 %next, %b
; UNCOND-NEXT: [[NEXT_REM:%.*]] = urem i32 %next, %b
; UNCOND-NEXT: [[NEXT_NESTED_QUO:%.*]] = udiv i32 [[NEXT_QUO]], %c
; UNCOND-NEXT: [[NEXT_NESTED_REM:%.*]] = urem i32 [[NEXT_QUO]], %c

; - DivRemIncrementCondBranchSimplify=1
; conditional branch simplify allowed
; COND: [[NEXT_TEST_OPT:%.*]] = icmp ule i32 16, %b
; COND-NEXT: br i1 [[NEXT_TEST_OPT]], label %[[SIMPLE:.*]], label %[[NORMAL:.*]]

; COND: [[SIMPLE]]:
; COND-NEXT: [[PREINC_REM:%.*]] = add i32 %rem, 16
; COND-NEXT: [[PREINC_REM_TEST:%.*]] = icmp uge i32 [[PREINC_REM]], %b
; COND-NEXT: [[PREINC_QUO:%.*]] = add i32 %quo, 1
; COND-NEXT: [[PREDEC_REM:%.*]] = sub i32 [[PREINC_REM]], %b
; COND-NEXT: [[SIMPLE_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %quo
; COND-NEXT: [[SIMPLE_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREDEC_REM]], i32 [[PREINC_REM]]
; COND-NEXT: [[PREINC_NESTED_REM:%.*]] = add i32 %nested_rem, 1
; COND-NEXT: [[PREINC_NESTED_REM_TEST:%.*]] = icmp eq i32 [[PREINC_NESTED_REM]], %c
; COND-NEXT: [[PREINC_NESTED_QUO:%.*]] = add i32 %nested_quo, 1
; COND-NEXT: [[NESTED_QUO_COND:%.*]] = and i1 [[PREINC_NESTED_REM_TEST]], [[PREINC_REM_TEST]]
; COND-NEXT: [[SIMPLE_NEXT_NESTED_QUO:%.*]] = select i1 [[NESTED_QUO_COND]], i32 [[PREINC_NESTED_QUO]], i32 %nested_quo
; COND-NEXT: [[SIMPLE_NEXT_NESTED_REM:%.*]] = select i1 [[PREINC_NESTED_REM_TEST]], i32 0, i32 [[PREINC_NESTED_REM]]
; COND-NEXT: [[MERGE_NEXT_NESTED_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[SIMPLE_NEXT_NESTED_REM]], i32 %nested_rem
; COND-NEXT: br label %[[JOIN:.*]]

; COND: [[NORMAL]]:
; COND-NEXT: [[NORMAL_NEXT_QUO:%.*]] = udiv i32 %next, %b
; COND-NEXT: [[NORMAL_NEXT_REM:%.*]] = urem i32 %next, %b
; COND-NEXT: [[NORMAL_NEXT_NESTED_QUO:%.*]] = udiv i32 [[NORMAL_NEXT_QUO]], %c
; COND-NEXT: [[NORMAL_NEXT_NESTED_REM:%.*]] = urem i32 [[NORMAL_NEXT_QUO]], %c
; COND-NEXT: br label %[[JOIN]]

; COND: [[JOIN]]:
; COND-NEXT: [[NEXT_QUO:%.*]] = phi i32 [ [[NORMAL_NEXT_QUO]], %[[NORMAL]] ], [ [[SIMPLE_NEXT_QUO]], %[[SIMPLE]] ]
; COND-NEXT: [[NEXT_REM:%.*]] = phi i32 [ [[NORMAL_NEXT_REM]], %[[NORMAL]] ], [ [[SIMPLE_NEXT_REM]], %[[SIMPLE]] ]
; COND-NEXT: [[NEXT_NESTED_QUO:%.*]] = phi i32 [ [[NORMAL_NEXT_NESTED_QUO]], %[[NORMAL]] ], [ [[SIMPLE_NEXT_NESTED_QUO]], %[[SIMPLE]] ]
; COND-NEXT: [[NEXT_NESTED_REM:%.*]] = phi i32 [ [[NORMAL_NEXT_NESTED_REM]], %[[NORMAL]] ], [ [[MERGE_NEXT_NESTED_REM]], %[[SIMPLE]] ]
  %next_quo = udiv i32 %next, %b
  %next_rem = urem i32 %next, %b
  %next_nested_quo = udiv i32 %next_quo, %c
  %next_nested_rem = urem i32 %next_quo, %c

  %next.2 = add i32 %a, 17
; Next.2 DivRemGroup Optimization, same for COND and UNCOND
; UNCOND: [[PREINC2_REM:%.*]] = add i32 [[NEXT_REM]], 1
; UNCOND-NEXT: [[PREINC2_REM_TEST:%.*]] = icmp eq i32 [[PREINC2_REM]], %b
; UNCOND-NEXT: [[PREINC2_QUO:%.*]] = add i32 [[NEXT_QUO]], 1
; UNCOND-NEXT: [[NEXT2_QUO:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 [[PREINC2_QUO]], i32 [[NEXT_QUO]]
; UNCOND-NEXT: [[NEXT2_REM:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 0, i32 [[PREINC2_REM]]

; COND: [[PREINC2_REM:%.*]] = add i32 [[NEXT_REM]], 1
; COND-NEXT: [[PREINC2_REM_TEST:%.*]] = icmp eq i32 [[PREINC2_REM]], %b
; COND-NEXT: [[PREINC2_QUO:%.*]] = add i32 [[NEXT_QUO]], 1
; COND-NEXT: [[NEXT2_QUO:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 [[PREINC2_QUO]], i32 [[NEXT_QUO]]
; COND-NEXT: [[NEXT2_REM:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 0, i32 [[PREINC2_REM]]
  %next.2_quo = udiv i32 %next.2, %b
  %next.2_rem = urem i32 %next.2, %b
; Next.2 Nested optimization, same for COND and UNCOND
; UNCOND: [[PREINC2_NESTED_REM:%.*]] = add i32 [[NEXT_NESTED_REM]], 1
; UNCOND-NEXT: [[PREINC2_NESTED_REM_TEST:%.*]] = icmp eq i32 [[PREINC2_NESTED_REM]], %c
; UNCOND-NEXT: [[PREINC2_NESTED_QUO:%.*]] = add i32 [[NEXT_NESTED_QUO]], 1
; UNCOND-NEXT: [[NESTED2_QUO_COND:%.*]] = and i1 [[PREINC2_NESTED_REM_TEST]], [[PREINC2_REM_TEST]]
; UNCOND-NEXT: [[NEXT2_NESTED_QUO:%.*]] = select i1 [[NESTED2_QUO_COND]], i32 [[PREINC2_NESTED_QUO]], i32 [[NEXT_NESTED_QUO]]
; UNCOND-NEXT: [[NEXT2_NESTED_REM:%.*]] = select i1 [[PREINC2_NESTED_REM_TEST]], i32 0, i32 [[PREINC2_NESTED_REM]]
; UNCOND-NEXT: [[MERGE_NEXT2_NESTED_REM:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 [[NEXT2_NESTED_REM]], i32 [[NEXT_NESTED_REM]]

; COND: [[PREINC2_NESTED_REM:%.*]] = add i32 [[NEXT_NESTED_REM]], 1
; COND-NEXT: [[PREINC2_NESTED_REM_TEST:%.*]] = icmp eq i32 [[PREINC2_NESTED_REM]], %c
; COND-NEXT: [[PREINC2_NESTED_QUO:%.*]] = add i32 [[NEXT_NESTED_QUO]], 1
; COND-NEXT: [[NESTED2_QUO_COND:%.*]] = and i1 [[PREINC2_NESTED_REM_TEST]], [[PREINC2_REM_TEST]]
; COND-NEXT: [[NEXT2_NESTED_QUO:%.*]] = select i1 [[NESTED2_QUO_COND]], i32 [[PREINC2_NESTED_QUO]], i32 [[NEXT_NESTED_QUO]]
; COND-NEXT: [[NEXT2_NESTED_REM:%.*]] = select i1 [[PREINC2_NESTED_REM_TEST]], i32 0, i32 [[PREINC2_NESTED_REM]]
; COND-NEXT: [[MERGE_NEXT2_NESTED_REM:%.*]] = select i1 [[PREINC2_REM_TEST]], i32 [[NEXT2_NESTED_REM]], i32 [[NEXT_NESTED_REM]]

; Check that original next nested udiv/urem deleted, same for COND and UNCOND
; UNCOND-NOT: udiv
; UNCOND-NOT: urem
; COND-NOT: udiv
; COND-NOT: urem
  %next.2_nested_quo = udiv i32 %next.2_quo, %c
  %next.2_nested_rem = urem i32 %next.2_quo, %c
; Check that original uses replaced, same for COND and UNCOND
; UNCOND: store i32 [[NEXT_QUO]], ptr %dest1
; UNCOND-NEXT: store i32 [[NEXT_REM]], ptr %dest2
; UNCOND-NEXT: store i32 [[NEXT_NESTED_QUO]], ptr %dest3
; UNCOND-NEXT: store i32 [[NEXT_NESTED_REM]], ptr %dest4
; UNCOND-NEXT: store i32 [[NEXT2_QUO]], ptr %dest5
; UNCOND-NEXT: store i32 [[NEXT2_REM]], ptr %dest6
; UNCOND-NEXT: store i32 [[NEXT2_NESTED_QUO]], ptr %dest7
; UNCOND-NEXT: store i32 [[MERGE_NEXT2_NESTED_REM]], ptr %dest8

; COND: store i32 [[NEXT_QUO]], ptr %dest1
; COND-NEXT: store i32 [[NEXT_REM]], ptr %dest2
; COND-NEXT: store i32 [[NEXT_NESTED_QUO]], ptr %dest3
; COND-NEXT: store i32 [[NEXT_NESTED_REM]], ptr %dest4
; COND-NEXT: store i32 [[NEXT2_QUO]], ptr %dest5
; COND-NEXT: store i32 [[NEXT2_REM]], ptr %dest6
; COND-NEXT: store i32 [[NEXT2_NESTED_QUO]], ptr %dest7
; COND-NEXT: store i32 [[MERGE_NEXT2_NESTED_REM]], ptr %dest8
  store i32 %next_quo, ptr %dest1
  store i32 %next_rem, ptr %dest2
  store i32 %next_nested_quo, ptr %dest3
  store i32 %next_nested_rem, ptr %dest4
  store i32 %next.2_quo, ptr %dest5
  store i32 %next.2_rem, ptr %dest6
  store i32 %next.2_nested_quo, ptr %dest7
  store i32 %next.2_nested_rem, ptr %dest8
  ret void
}

