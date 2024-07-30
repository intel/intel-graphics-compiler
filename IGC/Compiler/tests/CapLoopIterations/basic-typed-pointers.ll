;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --cap-loop-iterations-pass -S < %s | FileCheck %s
; ------------------------------------------------
; CapLoopIterations
; ------------------------------------------------

; Test checks that force loop exit on count of MAX_UINT is added.

declare void @use_int(i32)

define void @test_func(i32 %a, i32 addrspace(1)* %ptr) {
; CHECK:  entry:
; CHECK:    [[ID:%.*]] = add i32 [[A:%.*]], 1
; CHECK:    br label %[[LOOP:.*]]
; CHECK:  [[LOOP]]:
; CHECK:    [[INDVAR:%.*]] = phi i32 [ [[ID]], [[ENTRY:%.*]] ], [ [[NEXTINDVAR:%.*]], %[[LOOP]] ]
;
; Add new iteration counter
;
; CHECK:    [[COUNTERPHI:%.*]] = phi i32 [ 0, [[ENTRY]] ], [ [[COUNTER:%.*]], %[[LOOP]] ]
; CHECK:    [[COUNTER]] = add i32 [[COUNTERPHI]], 1
;
; Limit possible iterations to MAX_UINT
;
; CHECK:    [[FORCELOOPEXIT:%.*]] = icmp eq i32 [[COUNTER]], -1
;
; CHECK:    [[CMP:%.*]] = icmp sle i32 [[INDVAR]], 0
; CHECK:    [[SEL:%.*]] = select i1 [[CMP]], i32 10, i32 20
; CHECK:    [[NEXTINDVAR]] = sub i32 [[SEL]], [[INDVAR]]
; CHECK:    [[COND:%.*]] = icmp ult i32 [[NEXTINDVAR]], [[ID]]
;
; Merge with original condition
;
; CHECK:    [[TMP0:%.*]] = xor i1 [[FORCELOOPEXIT]], true
; CHECK:    [[TMP1:%.*]] = and i1 [[TMP0]], [[COND]]
; CHECK:    br i1 [[TMP1]], label %[[LOOP]], label %[[LOOP_END:.*]]
; CHECK:  [[LOOP_END]]:
; CHECK:    ret void
;
entry:
  %id = add i32 %a, 1
  br label %loop

loop:
  %indvar = phi i32 [ %id, %entry ], [ %nextindvar, %loop ]
  %cmp = icmp sle i32 %indvar, 0
  %sel = select i1 %cmp, i32 10, i32 20
  %nextindvar = sub i32 %sel, %indvar
  %cond = icmp ult i32 %nextindvar, %id
  br i1 %cond, label %loop, label %loop_end

loop_end:
  ret void
}
