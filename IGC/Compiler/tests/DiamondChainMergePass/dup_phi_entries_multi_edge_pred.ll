;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; repairSSAAfterMerge() runs over every PHI in the function once any merge
; happened.  A predecessor that reaches a block over several edges (here three
; switch cases sharing %sw.join) must keep one PHI entry per edge; collapsing
; them to one leaves getNumIncomingValues() < pred_size(), which the verifier
; rejects and which later crashes BasicBlock::removePredecessor().
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

define void @dup_phi_entries_multi_edge_pred(i1 %cmp, i32 %x, i8 %sel, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @dup_phi_entries_multi_edge_pred(
; CHECK:       sw.join:
; CHECK-NEXT:    %p = phi i32 [ 0, %sw.default ], [ 1, %sw.head ], [ 1, %sw.head ], [ 1, %sw.head ]
entry:
  br i1 %cmp, label %true0, label %false0

true0:
  %t0 = add i32 %x, 10
  br label %merge0

false0:
  %f0 = add i32 %x, 20
  br label %merge0

merge0:
  %v0 = phi i32 [ %t0, %true0 ], [ %f0, %false0 ]
  br i1 %cmp, label %true1, label %false1

true1:
  %t1 = add i32 %v0, 100
  br label %merge1

false1:
  %f1 = add i32 %v0, 200
  br label %merge1

merge1:
  %v1 = phi i32 [ %t1, %true1 ], [ %f1, %false1 ]
  store i32 %v1, ptr addrspace(1) %out, align 4
  br label %sw.head

sw.head:
  switch i8 %sel, label %sw.default [
    i8 1, label %sw.join
    i8 2, label %sw.join
    i8 3, label %sw.join
  ]

sw.default:
  br label %sw.join

sw.join:
  %p = phi i32 [ 0, %sw.default ], [ 1, %sw.head ], [ 1, %sw.head ], [ 1, %sw.head ]
  store i32 %p, ptr addrspace(1) %out, align 4
  ret void
}
