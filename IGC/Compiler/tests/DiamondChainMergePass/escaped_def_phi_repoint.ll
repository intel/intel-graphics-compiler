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
; The accumulator has a merge block (%if.end) and the source triple leaves it
; through two unrelated continuations, so there is no common exit block: %crit
; feeds %exit and %pre feeds the %loop header.
;
; %m is defined in the accumulator merge block, so it is cloned into both
; accumulators, and it is used by a PHI on each continuation. mergeBasicBlocks()
; repoints those PHI entries from the consumed %crit / %pre to %if.then /
; %if.else, but the entries still name the original %m, which the merge is about
; to delete. Each entry reaches its block over exactly one path, so it must be
; pointed at that path's clone - not at a fresh PHI, and not at poison.
;
; Reduced from a flash-attention kernel where the values lost this way were the
; pipeline phase counters driving barrier parity state.

define spir_kernel void @escaped_def_phi_repoint(i32 %n, ptr addrspace(1) %q) #0 {
; CHECK-LABEL: define spir_kernel void @escaped_def_phi_repoint(
; CHECK-NOT: poison
;
; CHECK: if.then:
; CHECK: %[[T:.*]] = add i32 %n, 1
; CHECK: %[[MT:.*]] = add i32 %[[T]], 3
; CHECK: br label %exit
;
; CHECK: if.else:
; CHECK: %[[F:.*]] = add i32 %n, 2
; CHECK: %[[MF:.*]] = add i32 %[[F]], 3
; CHECK: br label %loop
;
; Each continuation keeps the clone belonging to the path that reaches it.
; CHECK: loop:
; CHECK: %acc = phi i32 [ %[[MF]], %if.else ], [ %next, %latch ]
;
; CHECK: exit:
; CHECK: %r = phi i32 [ %[[MT]], %if.then ], [ %acc, %loop ]
; CHECK: store i32 %r
; CHECK: ret void
;
entry:
  %cond = icmp slt i32 %n, 16
  br i1 %cond, label %if.then, label %if.else

if.then:                                           ; preds = %entry
  %t = add i32 %n, 1
  br label %if.end

if.else:                                           ; preds = %entry
  %f = add i32 %n, 2
  br label %if.end

if.end:                                            ; preds = %if.then, %if.else
  %v = phi i32 [ %t, %if.then ], [ %f, %if.else ]
  %m = add i32 %v, 3
  br i1 %cond, label %crit, label %pre

crit:                                              ; preds = %if.end
  br label %exit

pre:                                               ; preds = %if.end
  br label %loop

loop:                                              ; preds = %pre, %latch
  %acc = phi i32 [ %m, %pre ], [ %next, %latch ]
  %next = add i32 %acc, 1
  %c2 = icmp slt i32 %next, %n
  br i1 %c2, label %latch, label %exit

latch:                                             ; preds = %loop
  br label %loop

exit:                                              ; preds = %crit, %loop
  %r = phi i32 [ %m, %crit ], [ %acc, %loop ]
  store i32 %r, ptr addrspace(1) %q, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
