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
; whole_triangle_chain.ll covers the fast path where every triple in the chain
; is a triangle and the whole chain collapses in one step.  Here the first two
; triples are triangles - so the chain does qualify for that path and its shape
; validation does run - but the last triple is a real diamond: %t2 and %f2 join
; in %m2 instead of the body block flowing straight into the next merge block.
; The whole-chain attempt has to bail out on that last pair, after which the
; ordinary per-triple merge takes over and still flattens the chain into the
; two paths below.

define spir_kernel void @chain_tail_is_diamond(i32 %x, ptr addrspace(1) align 4 %out) #0 {
; CHECK-LABEL: define spir_kernel void @chain_tail_is_diamond(
; CHECK-NOT:   cond.false.1:
; CHECK-NOT:   cond.end.1:
; CHECK-NOT:   %t2:
; CHECK-NOT:   %f2:
; CHECK-NOT:   %m2:
; CHECK:       entry:
; CHECK-NEXT:    %cmp = icmp eq i32 %x, 0
; CHECK-NEXT:    br i1 %cmp, label %cond.end, label %cond.false
; CHECK:       cond.false:
; CHECK:         %[[V1T:.*]] = add i32 %x, 20
; CHECK-NEXT:    %[[V2T:.*]] = add i32 %x, 30
; CHECK-NEXT:    %[[SUMT:.*]] = add i32 %[[V1T]], %[[V2T]]
; CHECK-NEXT:    store i32 %[[SUMT]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK:       cond.end:
; CHECK-NEXT:    %[[V2F:.*]] = add i32 %x, 40
; CHECK-NEXT:    %[[SUMF:.*]] = add i32 0, %[[V2F]]
; CHECK-NEXT:    store i32 %[[SUMF]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void

entry:
  %cmp = icmp eq i32 %x, 0
  br i1 %cmp, label %cond.end, label %cond.false

cond.false:
  %v0.body = add i32 %x, 10
  br label %cond.end

cond.end:
  %v0 = phi i32 [ %v0.body, %cond.false ], [ 0, %entry ]
  br i1 %cmp, label %cond.end.1, label %cond.false.1

cond.false.1:
  %v1.body = add i32 %x, 20
  br label %cond.end.1

cond.end.1:
  %v1 = phi i32 [ %v1.body, %cond.false.1 ], [ %v0, %cond.end ]
  br i1 %cmp, label %f2, label %t2

t2:
  %a = add i32 %x, 30
  br label %m2

f2:
  %b = add i32 %x, 40
  br label %m2

m2:
  %v2 = phi i32 [ %a, %t2 ], [ %b, %f2 ]
  %sum = add i32 %v1, %v2
  store i32 %sum, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
