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
; The accumulator triple (%dst.true/%dst.false/%join.0) is well formed, so the
; chain-growing loop is entered, but the *source* true block %src.true has two
; predecessors (%join.0 and %alt.path). The source-shape check rejects the
; triple, so no merge happens. Existing tests only cover the same rejection on
; the accumulator side, which bails out before the source loop is reached.

define spir_kernel void @multi_pred_src_true_skip(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @multi_pred_src_true_skip(
; CHECK:       entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 %x, 0
; CHECK-NEXT:    [[GATE:%.*]] = icmp eq i32 %x, 42
; CHECK-NEXT:    br i1 [[GATE]], label %dispatch, label %alt.path
; CHECK:       dispatch:
; CHECK-NEXT:    br i1 [[CMP]], label %dst.true, label %dst.false
; CHECK:       alt.path:
; CHECK-NEXT:    br label %src.true
; CHECK:       dst.true:
; CHECK-NEXT:    [[T0:%.*]] = add i32 %x, 10
; CHECK-NEXT:    br label %join.0
; CHECK:       dst.false:
; CHECK-NEXT:    [[F0:%.*]] = sub i32 %x, 10
; CHECK-NEXT:    br label %join.0
;
; The second branch on [[CMP]] must survive: nothing was inlined into the
; accumulator blocks above, and %src.true keeps its own body below.
; CHECK:       join.0:
; CHECK-NEXT:    [[M0:%.*]] = phi i32 [ [[T0]], %dst.true ], [ [[F0]], %dst.false ]
; CHECK-NEXT:    br i1 [[CMP]], label %src.true, label %src.false
; CHECK:       src.true:
; CHECK-NEXT:    [[P1:%.*]] = phi i32 [ [[M0]], %join.0 ], [ %x, %alt.path ]
; CHECK-NEXT:    [[T1:%.*]] = add i32 [[P1]], 1
; CHECK-NEXT:    br label %join.1
; CHECK:       src.false:
; CHECK-NEXT:    [[F1:%.*]] = sub i32 [[M0]], 1
; CHECK-NEXT:    br label %join.1
; CHECK:       join.1:
; CHECK-NEXT:    [[M1:%.*]] = phi i32 [ [[T1]], %src.true ], [ [[F1]], %src.false ]
; CHECK-NEXT:    store i32 [[M1]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:   .dcm
;
entry:
  %cmp = icmp sgt i32 %x, 0
  %gate = icmp eq i32 %x, 42
  br i1 %gate, label %dispatch, label %alt.path

dispatch:                                          ; preds = %entry
  br i1 %cmp, label %dst.true, label %dst.false

alt.path:                                          ; preds = %entry
  br label %src.true

dst.true:                                          ; preds = %dispatch
  %t0 = add i32 %x, 10
  br label %join.0

dst.false:                                         ; preds = %dispatch
  %f0 = sub i32 %x, 10
  br label %join.0

join.0:                                            ; preds = %dst.true, %dst.false
  %m0 = phi i32 [ %t0, %dst.true ], [ %f0, %dst.false ]
  br i1 %cmp, label %src.true, label %src.false

src.true:                                          ; preds = %join.0, %alt.path
  %p1 = phi i32 [ %m0, %join.0 ], [ %x, %alt.path ]
  %t1 = add i32 %p1, 1
  br label %join.1

src.false:                                         ; preds = %join.0
  %f1 = sub i32 %m0, 1
  br label %join.1

join.1:                                            ; preds = %src.true, %src.false
  %m1 = phi i32 [ %t1, %src.true ], [ %f1, %src.false ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
