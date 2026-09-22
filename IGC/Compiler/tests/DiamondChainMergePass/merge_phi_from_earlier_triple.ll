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
; Fold-by-path resolution across triples.  When a diamond's merge PHI is folded
; into one of the two paths, the incoming value for that path is normally either
; a fresh instruction of the same triple or something defined outside the chain.
; Here the second diamond's merge PHI takes %u0 - a value defined in the *first*
; diamond's merge block - on its true edge.  By the time the second triple is
; merged, %u0 has already been cloned into both paths, so the fold has to pick
; up the per-path clone recorded while merging the earlier triple rather than
; the original value, which would no longer be available on either path.

declare void @sink(i32)

define void @merge_phi_reuses_earlier_triple_value(i1 %c, i32 %x,
                                                   ptr addrspace(1) %p0,
                                                   ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @merge_phi_reuses_earlier_triple_value(
; CHECK-NOT:   %m0:
; CHECK-NOT:   %m1:
; CHECK-NOT:     phi
; CHECK:       t0:
; CHECK-NEXT:    %a0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    %[[U0T:.*]] = mul i32 %a0, 3
; CHECK-NEXT:    %{{.*}} = load i32, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    %[[ST:.*]] = add i32 %[[U0T]], %x
; CHECK-NEXT:    call void @sink(i32 %[[ST]])
; CHECK-NEXT:    ret void
; CHECK:       f0:
; CHECK-NEXT:    %b0 = add i32 %x, 1
; CHECK-NEXT:    %{{.*}} = mul i32 %b0, 3
; CHECK-NEXT:    %[[B1F:.*]] = add i32 %x, 2
; CHECK-NEXT:    %[[SF:.*]] = add i32 %[[B1F]], %x
; CHECK-NEXT:    call void @sink(i32 %[[SF]])
; CHECK-NEXT:    ret void

entry:
  br i1 %c, label %t0, label %f0

t0:
  %a0 = load i32, ptr addrspace(1) %p0, align 4
  br label %m0

f0:
  %b0 = add i32 %x, 1
  br label %m0

m0:
  %v0 = phi i32 [ %a0, %t0 ], [ %b0, %f0 ]
  %u0 = mul i32 %v0, 3
  br i1 %c, label %t1, label %f1

t1:
  %a1 = load i32, ptr addrspace(1) %p1, align 4
  br label %m1

f1:
  %b1 = add i32 %x, 2
  br label %m1

m1:
  %v1 = phi i32 [ %u0, %t1 ], [ %b1, %f1 ]
  %s = add i32 %v1, %x
  call void @sink(i32 %s)
  ret void
}
