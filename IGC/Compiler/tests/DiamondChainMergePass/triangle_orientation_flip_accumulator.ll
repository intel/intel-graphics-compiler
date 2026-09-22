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
; Regression: accumulator branch has reversed triangle orientation (succ1->succ0),
; while the next branch is a regular diamond. The pass must preserve path
; semantics after canonicalization and merging.

define spir_kernel void @triangle_orientation_flip_accumulator(
    i1 %c, ptr addrspace(1) align 4 captures(none) %out) {
; CHECK-LABEL: define spir_kernel void @triangle_orientation_flip_accumulator(
; CHECK-SAME: i1 [[C:%.*]], ptr addrspace(1) align 4 captures(none) [[OUT:%.*]]) {
; CHECK:       entry:
; CHECK-NEXT:    br i1 [[C]], label %[[TAIL0:.*]], label %[[MID0:.*]]
; CHECK-NOT:   t1:
; CHECK-NOT:   f1:
; CHECK-NOT:   merge1:
;
; c=false path: via mid0 payload (7), then false payload (13), then final store (22).
; CHECK:       [[MID0]]:
; CHECK:         store i32 7, ptr addrspace(1) [[OUT]], align 4
; CHECK:         [[IDX1:%.*]] = sext i32 1 to i64
; CHECK:         [[P1:%.*]] = getelementptr inbounds i32, ptr addrspace(1) [[OUT]], i64 [[IDX1]]
; CHECK:         store i32 13, ptr addrspace(1) [[P1]], align 4
; CHECK:         store i32 22, ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
; c=true path: direct to tail0, then true payload (11), then final store (22).
; CHECK:       [[TAIL0]]:
; CHECK:         [[IDX0:%.*]] = sext i32 0 to i64
; CHECK:         [[P0:%.*]] = getelementptr inbounds i32, ptr addrspace(1) [[OUT]], i64 [[IDX0]]
; CHECK:         store i32 11, ptr addrspace(1) [[P0]], align 4
; CHECK:         store i32 22, ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
entry:
  ; Reversed triangle orientation in accumulator branch: succ1 flows to succ0.
  br i1 %c, label %tail0, label %mid0

mid0:
  store i32 7, ptr addrspace(1) %out, align 4
  br label %tail0

tail0:
  %x = phi i32 [ 0, %entry ], [ 1, %mid0 ]
  %idx = sext i32 %x to i64
  %p = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
  ; Regular diamond on the same condition.
  br i1 %c, label %t1, label %f1

t1:
  store i32 11, ptr addrspace(1) %p, align 4
  br label %merge1

f1:
  store i32 13, ptr addrspace(1) %p, align 4
  br label %merge1

merge1:
  store i32 22, ptr addrspace(1) %out, align 4
  ret void
}
