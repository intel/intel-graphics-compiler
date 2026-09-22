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
; Regression: reject accumulator triangle when false sink has 3 predecessors.
; Here %dst.false has preds from %dispatch, %dst.true, and %extra.edge.

define spir_kernel void @false_three_preds_skip(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @false_three_preds_skip(
; CHECK:       entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 %x, 0
; CHECK-NEXT:    [[GATE:%.*]] = icmp eq i32 %x, 42
; CHECK-NEXT:    br i1 [[GATE]], label %dispatch, label %extra.edge
; CHECK:       dispatch:
; CHECK-NEXT:    br i1 [[CMP]], label %dst.true, label %dst.false
; CHECK:       extra.edge:
; CHECK-NEXT:    br label %dst.false
; CHECK:       dst.true:
; CHECK-NEXT:    [[T0:%.*]] = add i32 %x, 7
; CHECK-NEXT:    br label %dst.false
; CHECK:       dst.false:
; CHECK-NEXT:    [[M0:%.*]] = phi i32 [ %x, %dispatch ], [ [[T0]], %dst.true ], [ %x, %extra.edge ]
; CHECK-NEXT:    br i1 [[CMP]], label %src.true, label %src.false
; CHECK:       src.true:
; CHECK-NEXT:    [[T1:%.*]] = add i32 [[M0]], 1
; CHECK-NEXT:    br label %src.false
; CHECK:       src.false:
; CHECK-NEXT:    [[M1:%.*]] = phi i32 [ [[M0]], %dst.false ], [ [[T1]], %src.true ]
; CHECK-NEXT:    store i32 [[M1]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:   .dcm = phi
;
entry:
  %cmp = icmp sgt i32 %x, 0
  %gate = icmp eq i32 %x, 42
  br i1 %gate, label %dispatch, label %extra.edge

dispatch:
  br i1 %cmp, label %dst.true, label %dst.false

extra.edge:
  br label %dst.false

dst.true:                                          ; preds = %dispatch
  %t0 = add i32 %x, 7
  br label %dst.false

dst.false:                                         ; preds = %extra.edge, %dst.true, %dispatch
  %m0 = phi i32 [ %x, %dispatch ], [ %t0, %dst.true ], [ %x, %extra.edge ]
  br i1 %cmp, label %src.true, label %src.false

src.true:                                          ; preds = %dst.false
  %t1 = add i32 %m0, 1
  br label %src.false

src.false:                                         ; preds = %src.true, %dst.false
  %m1 = phi i32 [ %m0, %dst.false ], [ %t1, %src.true ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
