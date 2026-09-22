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
;
; Regression for debug_output-style triangle merge.
; Ensure transformed true-path computes t1 from t0 (not poison), and false-path
; still stores x through merged PHI semantics.

define spir_kernel void @debug_output_semantics(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @debug_output_semantics(
; CHECK-SAME: i32 [[X:%.*]], ptr addrspace(1) [[OUT:%.*]]) #0 {
; CHECK:       entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 [[X]], 0
; CHECK-NEXT:    br i1 [[CMP]], label %[[TRUE_PATH:.*]], label %[[MERGE_PATH:.*]]
; CHECK-NOT:   poison
; CHECK:       [[TRUE_PATH]]:
; CHECK-NEXT:    [[T0:%.*]] = add i32 [[X]], 1
; CHECK-NEXT:    [[T1:%.*]] = add i32 [[T0]], 2
; CHECK-NEXT:    br label %[[MERGE_PATH]]
; CHECK:       [[MERGE_PATH]]:
; CHECK-NEXT:    [[M1:%.*]] = phi i32 [ [[X]], %entry ], [ [[T1]], %[[TRUE_PATH]] ]
; CHECK:         store i32 [[M1]], ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %dst.true, label %dst.false

dst.true:                                          ; preds = %entry
  %t0 = add i32 %x, 1
  br label %dst.false

dst.false:                                         ; preds = %dst.true, %entry
  %m0 = phi i32 [ %x, %entry ], [ %t0, %dst.true ]
  br i1 %cmp, label %src.true, label %src.false

src.true:                                          ; preds = %dst.false
  %t1 = add i32 %m0, 2
  br label %src.false

src.false:                                         ; preds = %src.true, %dst.false
  %m1 = phi i32 [ %m0, %dst.false ], [ %t1, %src.true ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
