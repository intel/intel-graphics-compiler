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
; A value defined inside the merged chain (%e, in merge2) is used past the end
; of the chain (in tail).  After merging it exists as two independent clones -
; one per path - so the use must be served by a PHI at the point where body0
; and merge0 converge.  Previously check4 rejected the second triple over this
; escape, the chain was trimmed to a one-triple prefix, and the values feeding
; the surviving merge2 (%a0 from merge0, %v1 from merge1) degraded to poison.

define spir_kernel void @escaped_def_phi_repair(ptr addrspace(1) %p, ptr addrspace(1) %q, i32 %n) #0 {
; CHECK-LABEL: define spir_kernel void @escaped_def_phi_repair(
; CHECK-NOT: poison
;
; The whole chain collapses onto the two accumulator blocks.
; CHECK: body0:
; CHECK: %[[L0:.*]] = load i32
; CHECK: %[[A0T:.*]] = add i32 %[[L0]], 1
; CHECK: %[[L1:.*]] = load i32
; CHECK: %[[L2:.*]] = load i32
; CHECK: store i32 %[[A0T]]
; CHECK: store i32 %[[L1]]
; CHECK: store i32 %[[L2]]
; CHECK: %[[ET:.*]] = add i32 %[[L2]], 1
; CHECK: br label %tail
;
; CHECK: merge0:
; CHECK: %[[A0F:.*]] = add i32 0, 1
; CHECK: store i32 %[[A0F]]
; CHECK: store i32 0
; CHECK: store i32 0
; CHECK: %[[EF:.*]] = add i32 0, 1
; CHECK: br label %tail
;
; The escaping value is reconstructed by a PHI in the convergence block.
; CHECK: tail:
; CHECK: %[[E:.*]] = phi i32 [ %[[EF]], %merge0 ], [ %[[ET]], %body0 ]
; CHECK: store i32 %[[E]]
; CHECK: ret void
;
entry:
  %cond = icmp slt i32 %n, 16
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
  %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
  br i1 %cond, label %body2, label %merge2

body2:                                             ; preds = %merge1
  %l2 = load i32, ptr addrspace(1) %p, align 4
  br label %merge2

merge2:                                            ; preds = %body2, %merge1
  %v2 = phi i32 [ %l2, %body2 ], [ 0, %merge1 ]
  store i32 %a0, ptr addrspace(1) %q, align 4
  store i32 %v1, ptr addrspace(1) %q, align 4
  store i32 %v2, ptr addrspace(1) %q, align 4
  %e = add i32 %v2, 1
  br label %tail

tail:                                              ; preds = %merge2
  store i32 %e, ptr addrspace(1) %q, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
