;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

define spir_kernel void @whole_triangle_chain(i32 %x, ptr addrspace(1) align 4 captures(none) %out) #0 {
; CHECK-LABEL: define spir_kernel void @whole_triangle_chain(
; CHECK-SAME: i32 [[X:%.*]], ptr addrspace(1) align 4 captures(none) [[OUT:%.*]]) #0 {
; CHECK:       entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp eq i32 [[X]], 0
; CHECK-NEXT:    br i1 [[CMP]], label %[[MERGE:cond.end.2]], label %[[BODY:cond.false.2]]
; CHECK-NOT:   cond.false:
; CHECK-NOT:   cond.end:
; CHECK-NOT:   cond.false.1:
; CHECK-NOT:   cond.end.1:
; CHECK:       [[BODY]]:
; CHECK-DAG:     [[V2BODY:%.*]] = add i32 [[X]], 30
; CHECK-DAG:     [[V0BODY:%.*]] = add i32 [[X]], 10
; CHECK-DAG:     [[V1BODY:%.*]] = add i32 [[X]], 20
; CHECK-NEXT:    br label %[[MERGE]]
; CHECK:       [[MERGE]]:
; CHECK-DAG:     [[V2:%.*]] = phi i32 [ [[V2BODY]], %[[BODY]] ], [ 0, %entry ]
; CHECK-DAG:     [[V0:%.*]] = phi i32 [ [[V0BODY]], %[[BODY]] ], [ 0, %entry ]
; CHECK-DAG:     [[V1:%.*]] = phi i32 [ [[V1BODY]], %[[BODY]] ], [ 0, %entry ]
; CHECK:         [[SUM0:%.*]] = add i32 [[V0]], [[V1]]
; CHECK-NEXT:    [[SUM1:%.*]] = add i32 [[SUM0]], [[V2]]
; CHECK-NEXT:    store i32 [[SUM1]], ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
entry:
  %cmp = icmp eq i32 %x, 0
  br i1 %cmp, label %cond.end, label %cond.false

cond.false:                                        ; preds = %entry
  %v0.body = add i32 %x, 10
  br label %cond.end

cond.end:                                          ; preds = %cond.false, %entry
  %v0 = phi i32 [ %v0.body, %cond.false ], [ 0, %entry ]
  br i1 %cmp, label %cond.end.1, label %cond.false.1

cond.false.1:                                      ; preds = %cond.end
  %v1.body = add i32 %x, 20
  br label %cond.end.1

cond.end.1:                                        ; preds = %cond.false.1, %cond.end
  %v1 = phi i32 [ %v1.body, %cond.false.1 ], [ %v0, %cond.end ]
  br i1 %cmp, label %cond.end.2, label %cond.false.2

cond.false.2:                                      ; preds = %cond.end.1
  %v2.body = add i32 %x, 30
  br label %cond.end.2

cond.end.2:                                        ; preds = %cond.false.2, %cond.end.1
  %v2 = phi i32 [ %v2.body, %cond.false.2 ], [ %v1, %cond.end.1 ]
  %sum0 = add i32 %v0, %v1
  %sum1 = add i32 %sum0, %v2
  store i32 %sum1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
