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
; Regression: preserve a side-effecting sink call while flattening a diamond
; chain. The pass should fully flatten the chain and keep the sink call on the
; two surviving paths.

declare void @sink(i32, i32, i32)

define void @f(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1,
               ptr addrspace(1) %p2, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @f(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %[[L0:.*]] = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    %[[A0:.*]] = add i32 %[[L0]], 1
; CHECK-NEXT:    %[[L1:.*]] = load i32, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    %[[A1:.*]] = add i32 %[[L1]], 1
; CHECK-NEXT:    %[[L2:.*]] = load i32, ptr addrspace(1) %p2, align 4
; CHECK-NEXT:    %[[A2:.*]] = add i32 %[[L2]], 1
; CHECK-NEXT:    call void @sink(i32 %[[A0]], i32 %[[A1]], i32 %[[A2]])
; CHECK-NEXT:    ret void
; CHECK:       merge0:
; CHECK-NEXT:    %[[U0:.*]] = add i32 0, 1
; CHECK-NEXT:    %[[U1:.*]] = add i32 0, 1
; CHECK-NEXT:    %[[U2:.*]] = add i32 0, 1
; CHECK-NEXT:    call void @sink(i32 %[[U0]], i32 %[[U1]], i32 %[[U2]])
; CHECK-NEXT:    ret void
; CHECK-NOT:   body1:
; CHECK-NOT:   merge1:
; CHECK-NOT:   body2:
; CHECK-NOT:   merge2:

entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  %u0 = add i32 %v0, 1
  br i1 %c, label %body1, label %merge1

body1:
  %l1 = load i32, ptr addrspace(1) %p1, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
  %u1 = add i32 %v1, 1
  br i1 %c, label %body2, label %merge2

body2:
  %l2 = load i32, ptr addrspace(1) %p2, align 4
  br label %merge2

merge2:
  %v2 = phi i32 [ %l2, %body2 ], [ 0, %merge1 ]
  %u2 = add i32 %v2, 1
  call void @sink(i32 %u0, i32 %u1, i32 %u2)
  ret void
}
