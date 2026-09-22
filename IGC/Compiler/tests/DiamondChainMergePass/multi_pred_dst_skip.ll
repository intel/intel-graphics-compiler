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
; Regression case: the first grouped branch has destination %dst.true with
; multiple predecessors (%dispatch and %alt.path). Such CFG shape must be
; rejected during candidate collection.

define spir_kernel void @multi_pred_dst_skip(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @multi_pred_dst_skip(
; CHECK:       entry:
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i32 %x, 0
; CHECK-NEXT:    [[GATE:%.*]] = icmp eq i32 %x, 42
; CHECK-NEXT:    br i1 [[GATE]], label %dispatch, label %alt.path
; CHECK:       dispatch:
; CHECK-NEXT:    br i1 [[CMP]], label %dst.true, label %dst.false
; CHECK:       alt.path:
; CHECK-NEXT:    br label %dst.true
; CHECK:       dst.true:
; CHECK-NEXT:    [[T0:%.*]] = add i32 %x, 10
; CHECK-NEXT:    br label %join.0
; CHECK:       dst.false:
; CHECK-NEXT:    [[F0:%.*]] = sub i32 %x, 10
; CHECK-NEXT:    br label %join.0
; CHECK:       join.0:
; CHECK-NEXT:    [[M0:%.*]] = phi i32 [ [[T0]], %dst.true ], [ [[F0]], %dst.false ]
; CHECK-NEXT:    br i1 [[CMP]], label %src.true, label %src.false
; CHECK:       src.true:
; CHECK-NEXT:    [[T1:%.*]] = add i32 [[M0]], 1
; CHECK-NEXT:    br label %join.1
; CHECK:       src.false:
; CHECK-NEXT:    [[F1:%.*]] = sub i32 [[M0]], 1
; CHECK-NEXT:    br label %join.1
; CHECK:       join.1:
; CHECK-NEXT:    [[M1:%.*]] = phi i32 [ [[T1]], %src.true ], [ [[F1]], %src.false ]
; CHECK-NEXT:    store i32 [[M1]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:   .dcm = phi
;
entry:
  %cmp = icmp sgt i32 %x, 0
  %gate = icmp eq i32 %x, 42
  br i1 %gate, label %dispatch, label %alt.path

dispatch:
  br i1 %cmp, label %dst.true, label %dst.false

alt.path:
  br label %dst.true

dst.true:                                          ; preds = %alt.path, %dispatch
  %t0 = add i32 %x, 10
  br label %join.0

dst.false:                                         ; preds = %dispatch
  %f0 = sub i32 %x, 10
  br label %join.0

join.0:                                            ; preds = %dst.false, %dst.true
  %m0 = phi i32 [ %t0, %dst.true ], [ %f0, %dst.false ]
  br i1 %cmp, label %src.true, label %src.false

src.true:                                          ; preds = %join.0
  %t1 = add i32 %m0, 1
  br label %join.1

src.false:                                         ; preds = %join.0
  %f1 = sub i32 %m0, 1
  br label %join.1

join.1:                                            ; preds = %src.false, %src.true
  %m1 = phi i32 [ %t1, %src.true ], [ %f1, %src.false ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
