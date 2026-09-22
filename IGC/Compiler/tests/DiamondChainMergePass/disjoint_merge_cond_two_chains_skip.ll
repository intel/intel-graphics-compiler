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
; Verification check 2 (mergeBlockBarrierWithDisjointCond): a candidate whose
; final merge block branches on a condition *different* from the one the
; diamonds branch on must be rejected, because the merge block is then a real
; join point and folding the chain into it would change control flow.
;
; Two independent chains are needed, not one.  Rejected candidates are removed
; from the candidate vector by swapping the last element down, and that swap is
; only performed when the rejected candidate is not already the last one.  With
; a single rejected candidate the vector simply shrinks and the compaction is
; never exercised.  The two chains must also branch on four *distinct* i1
; values: candidate discovery groups conditional branches by condition value, so
; sharing a condition between the chains folds them into a single candidate.

define void @two_disjoint_chains(i1 %c, i1 %d, i1 %e, i1 %f, i32 %x,
                                 ptr addrspace(1) %out) {
; CHECK-LABEL: define void @two_disjoint_chains(
; CHECK-NOT:     .dcm
; CHECK:       one.m0:
; CHECK-NEXT:    %one.p0 = phi i32 [ %one.a0, %one.t0 ], [ %one.s0, %one.f0 ]
; CHECK-NEXT:    br i1 %c, label %one.t1, label %one.f1
; CHECK:       one.m1:
; CHECK-NEXT:    %one.p1 = phi i32 [ %one.a1, %one.t1 ], [ %one.s1, %one.f1 ]
; CHECK-NEXT:    br i1 %d, label %one.x, label %one.y
; CHECK:       two.m0:
; CHECK-NEXT:    %two.p0 = phi i32 [ %two.a0, %two.t0 ], [ %two.s0, %two.f0 ]
; CHECK-NEXT:    br i1 %e, label %two.t1, label %two.f1
; CHECK:       two.m1:
; CHECK-NEXT:    %two.p1 = phi i32 [ %two.a1, %two.t1 ], [ %two.s1, %two.f1 ]
; CHECK-NEXT:    br i1 %f, label %two.x, label %two.y

entry:
  br label %one.entry

one.entry:
  br i1 %c, label %one.t0, label %one.f0

one.t0:
  %one.a0 = add i32 %x, 1
  br label %one.m0

one.f0:
  %one.s0 = sub i32 %x, 1
  br label %one.m0

one.m0:
  %one.p0 = phi i32 [ %one.a0, %one.t0 ], [ %one.s0, %one.f0 ]
  br i1 %c, label %one.t1, label %one.f1

one.t1:
  %one.a1 = add i32 %one.p0, 2
  br label %one.m1

one.f1:
  %one.s1 = sub i32 %one.p0, 2
  br label %one.m1

one.m1:
  %one.p1 = phi i32 [ %one.a1, %one.t1 ], [ %one.s1, %one.f1 ]
  br i1 %d, label %one.x, label %one.y

one.x:
  store i32 %one.p1, ptr addrspace(1) %out, align 4
  br label %one.done

one.y:
  store i32 0, ptr addrspace(1) %out, align 4
  br label %one.done

one.done:
  br label %two.entry

two.entry:
  br i1 %e, label %two.t0, label %two.f0

two.t0:
  %two.a0 = add i32 %x, 1
  br label %two.m0

two.f0:
  %two.s0 = sub i32 %x, 1
  br label %two.m0

two.m0:
  %two.p0 = phi i32 [ %two.a0, %two.t0 ], [ %two.s0, %two.f0 ]
  br i1 %e, label %two.t1, label %two.f1

two.t1:
  %two.a1 = add i32 %two.p0, 2
  br label %two.m1

two.f1:
  %two.s1 = sub i32 %two.p0, 2
  br label %two.m1

two.m1:
  %two.p1 = phi i32 [ %two.a1, %two.t1 ], [ %two.s1, %two.f1 ]
  br i1 %f, label %two.x, label %two.y

two.x:
  store i32 %two.p1, ptr addrspace(1) %out, align 4
  br label %two.done

two.y:
  store i32 0, ptr addrspace(1) %out, align 4
  br label %two.done

two.done:
  ret void
}
