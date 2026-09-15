;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The walk for the values two predicates share stops at a PHI. What is below a
; PHI that may be undef is unknown, so the reuse is rejected, unless the
; predicate reads the PHI only through a value that is frozen.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Both loads read the PHI %n only through the shared %q. Freezing %q makes every
; read of %n go through %q.fr, so %n needs no freeze.
; CHECK-LABEL: define i32 @shared_condition_above_phi(
; CHECK: %q = icmp ult i32 %n, 8
; CHECK-NEXT: %q.fr = freeze i1 %q
; CHECK-NEXT: %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT: %[[REPLACEMENT:.*]] = select i1 %q.fr, i32 %a, i32 13
; CHECK-NEXT: %sum = add i32 %a, %[[REPLACEMENT]]
; CHECK-NEXT: ret i32 %sum
define i32 @shared_condition_above_phi(ptr addrspace(1) %p, i1 noundef %branch, i32 %x, i32 %y) {
entry:
  br i1 %branch, label %left, label %right
left:
  br label %merge
right:
  br label %merge
merge:
  %n = phi i32 [ %x, %left ], [ %y, %right ]
  %q = icmp ult i32 %n, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %sum = add i32 %a, %b
  ret i32 %sum
}

; The candidate predicate %q1 also reads %n directly, through %k and not through
; the shared %q, so the reuse is rejected and nothing is frozen.
; CHECK-LABEL: define i32 @phi_also_read_outside_shared(
; CHECK-NOT:     freeze
; CHECK:         %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
define i32 @phi_also_read_outside_shared(ptr addrspace(1) %p, i1 noundef %branch, i32 %x, i32 %y) {
entry:
  br i1 %branch, label %left, label %right
left:
  br label %merge
right:
  br label %merge
merge:
  %n = phi i32 [ %x, %left ], [ %y, %right ]
  %q = icmp ult i32 %n, 8
  %k = icmp eq i32 %n, 100
  %q1 = or i1 %q, %k
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %sum = add i32 %a, %b
  ret i32 %sum
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
