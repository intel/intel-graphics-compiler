;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; When MergeBB has several edges to a PHI block, cloning its terminator into
; both diamond paths must create one PHI entry per cloned edge on each path.
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

define void @partial_duplicate_phi(i1 %cond, i8 %sel, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @partial_duplicate_phi(
; CHECK:       dst.true:
; CHECK:         switch i8 %sel, label %join [
; CHECK:       dst.false:
; CHECK:         switch i8 %sel, label %join [
; CHECK:       join:
; CHECK-NEXT:    %p = phi i32 [ 7, %dst.true ], [ 7, %dst.true ], [ 7, %dst.true ], [ 7, %dst.false ], [ 7, %dst.false ], [ 7, %dst.false ]
entry:
  br i1 %cond, label %dst.true, label %dst.false

dst.true:
  br label %dst.merge

dst.false:
  br label %dst.merge

dst.merge:
  br i1 %cond, label %src.true, label %src.false

src.true:
  br label %src.merge

src.false:
  br label %src.merge

src.merge:
  switch i8 %sel, label %join [
    i8 1, label %join
    i8 2, label %join
  ]

join:
  %p = phi i32 [ 7, %src.merge ], [ 7, %src.merge ], [ 7, %src.merge ]
  store i32 %p, ptr addrspace(1) %out
  ret void
}
