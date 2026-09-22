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
; The chain (one triple) exits through a conditional branch on an unrelated
; condition, so body0 and merge0 both end up branching to body2 and merge2 and
; the two paths only converge two blocks later.  Two things are exercised:
;
;   * %a0 (defined in merge0) and %v1 (defined in merge1) escape the chain and
;     need a *cascade* of PHIs - one in body2, one in merge2 - not a single
;     PHI at an immediate successor.
;   * %v2 already had an incoming entry for the exit block merge1.  Because
;     merge1's terminator is copied into both accumulators, that entry must be
;     split per path instead of renamed, otherwise the body0 edge is left
;     unrepresented and later filled with poison.

define spir_kernel void @escaped_def_phi_cascade(ptr addrspace(1) %p, ptr addrspace(1) %q, i32 %n, i32 %m) #0 {
; CHECK-LABEL: define spir_kernel void @escaped_def_phi_cascade(
; CHECK-NOT: poison
;
; CHECK: body0:
; CHECK: %[[L0:.*]] = load i32
; CHECK: %[[A0T:.*]] = add i32 %[[L0]], 1
; CHECK: %[[L1:.*]] = load i32
; CHECK: br i1 %cond2, label %body2, label %merge2
;
; CHECK: merge0:
; CHECK: %[[A0F:.*]] = add i32 0, 1
; CHECK: br i1 %cond2, label %body2, label %merge2
;
; First level of the cascade.
; CHECK: body2:
; CHECK: %[[V1B:.*]] = phi i32 [ 5, %merge0 ], [ %[[L1]], %body0 ]
; CHECK: %[[A0B:.*]] = phi i32 [ %[[A0F]], %merge0 ], [ %[[A0T]], %body0 ]
;
; Second level, plus the per-path split of the pre-existing %v2 PHI.
; CHECK: merge2:
; CHECK: %[[V1:.*]] = phi i32 [ %[[V1B]], %body2 ], [ %[[L1]], %body0 ], [ 5, %merge0 ]
; CHECK: %[[A0:.*]] = phi i32 [ %[[A0B]], %body2 ], [ %[[A0T]], %body0 ], [ %[[A0F]], %merge0 ]
; CHECK: %[[V2:.*]] = phi i32 [ %{{.*}}, %body2 ], [ 0, %body0 ], [ 0, %merge0 ]
; CHECK: store i32 %[[A0]]
; CHECK: store i32 %[[V1]]
; CHECK: store i32 %[[V2]]
; CHECK: ret void
;
entry:
  %cond = icmp slt i32 %n, 16
  %cond2 = icmp slt i32 %m, 16
  br i1 %cond, label %body0, label %merge0

body0:                                             ; preds = %entry
  %l0 = load i32, ptr addrspace(1) %p, align 4
  br label %merge0

merge0:                                            ; preds = %body0, %entry
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  %a0 = add i32 %v0, 1
  br i1 %cond, label %body1, label %merge1

body1:                                             ; preds = %merge0
  %l1 = load i32, ptr addrspace(1) %p, align 4
  br label %merge1

merge1:                                            ; preds = %body1, %merge0
  %v1 = phi i32 [ %l1, %body1 ], [ 5, %merge0 ]
  br i1 %cond2, label %body2, label %merge2

body2:                                             ; preds = %merge1
  %l2 = load i32, ptr addrspace(1) %p, align 4
  br label %merge2

merge2:                                            ; preds = %body2, %merge1
  %v2 = phi i32 [ %l2, %body2 ], [ 0, %merge1 ]
  store i32 %a0, ptr addrspace(1) %q, align 4
  store i32 %v1, ptr addrspace(1) %q, align 4
  store i32 %v2, ptr addrspace(1) %q, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
