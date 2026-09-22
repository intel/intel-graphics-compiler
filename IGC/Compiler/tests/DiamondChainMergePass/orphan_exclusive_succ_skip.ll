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
; Regression: reject chain merge when rewriting destination terminators would
; orphan destination-exclusive successors.
;
; dst.true and dst.false each own a single-predecessor successor (dst.keep.t /
; dst.keep.f). Merging src.true/src.false into destination must not replace
; destination terminators in a way that drops these blocks.

define spir_kernel void @orphan_exclusive_succ_skip(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @orphan_exclusive_succ_skip(
; CHECK:       dispatch:
; CHECK-NEXT:    br i1 [[CMP:%.*]], label %dst.true, label %dst.false
; CHECK:       dst.true:
; CHECK-NEXT:    [[T0:%.*]] = add i32 %x, 10
; CHECK-NEXT:    br i1 [[GT:%.*]], label %dst.keep.t, label %join.0
; CHECK:       dst.keep.t:
; CHECK-NEXT:    [[TK:%.*]] = add i32 [[T0]], 100
; CHECK-NEXT:    br label %join.0
; CHECK:       dst.false:
; CHECK-NEXT:    [[F0:%.*]] = sub i32 %x, 10
; CHECK-NEXT:    br i1 [[GT]], label %dst.keep.f, label %join.0
; CHECK:       dst.keep.f:
; CHECK-NEXT:    [[FK:%.*]] = sub i32 [[F0]], 100
; CHECK-NEXT:    br label %join.0
; CHECK:       join.0:
; CHECK-NEXT:    [[M0:%.*]] = phi i32 [ [[T0]], %dst.true ], [ [[TK]], %dst.keep.t ], [ [[F0]], %dst.false ], [ [[FK]], %dst.keep.f ]
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
  %gt = icmp sgt i32 %x, 5
  br label %dispatch

dispatch:
  br i1 %cmp, label %dst.true, label %dst.false

dst.true:
  %t0 = add i32 %x, 10
  br i1 %gt, label %dst.keep.t, label %join.0

dst.keep.t:
  %tk = add i32 %t0, 100
  br label %join.0

dst.false:
  %f0 = sub i32 %x, 10
  br i1 %gt, label %dst.keep.f, label %join.0

dst.keep.f:
  %fk = sub i32 %f0, 100
  br label %join.0

join.0:
  %m0 = phi i32 [ %t0, %dst.true ], [ %tk, %dst.keep.t ], [ %f0, %dst.false ], [ %fk, %dst.keep.f ]
  br i1 %cmp, label %src.true, label %src.false

src.true:
  %t1 = add i32 %m0, 1
  br label %join.1

src.false:
  %f1 = sub i32 %m0, 1
  br label %join.1

join.1:
  %m1 = phi i32 [ %t1, %src.true ], [ %f1, %src.false ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
