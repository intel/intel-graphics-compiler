;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; The predicate implies the condition the use rejects, so the load could be
; removed. The removal repeats the proof after freezing. Removal is rejected,
; and the predicate is shrunk instead.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Frozen, and the repeated proof fails. For %sum = add %x, %y, %q implies %c.
; The shared %sum and %x are frozen. The two freezes are chosen independently,
; so %sum.fr need not equal %x.fr + %y, and the `add` no longer relates the two
; comparisons. The proof fails, and both freezes are rolled back. The shrink
; then freezes only the condition.
; CHECK-LABEL: define i32 @frozen_proof_fails(
; CHECK-NOT:     %sum.fr
; CHECK-NOT:     %x.fr
; CHECK:         %c.fr = freeze i1 %c
; CHECK-NEXT:    %[[NOT:.+]] = xor i1 %c.fr, true
; CHECK-NEXT:    %[[MASK:.+]] = and i1 %q, %[[NOT]]
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %[[MASK]], i32 0)
; CHECK-NEXT:    %r = select i1 %c.fr, i32 %v, i32 %a

define i32 @frozen_proof_fails(ptr addrspace(1) %p, i32 %x, i32 %y, i32 %v) {
  %sum = add i32 %x, %y
  %q = icmp uge i32 %sum, %x
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %c = icmp uge i32 %sum, %y
  %r = select i1 %c, i32 %v, i32 %a
  ret i32 %r
}

; The freeze fails. %lt5 in %q implies %c, but the walk for shared values stops
; at the PHI %n, which may be undef. Nothing is frozen, and the predicate is
; shrunk instead.
; CHECK-LABEL: define i32 @freeze_fails(
; CHECK-NOT:     freeze
; CHECK:         %[[NOT:.+]] = xor i1 %c, true
; CHECK-NEXT:    %[[MASK:.+]] = and i1 %q, %[[NOT]]
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %[[MASK]], i32 0)
; CHECK-NEXT:    %r = select i1 %c, i32 %v, i32 %a

define i32 @freeze_fails(ptr addrspace(1) %p, i1 noundef %br, i32 noundef %u, i32 %x, i32 %y, i32 %v) {
entry:
  br i1 %br, label %left, label %right
left:
  br label %merge
right:
  br label %merge
merge:
  %n = phi i32 [ %x, %left ], [ %y, %right ]
  %lt5 = icmp ult i32 %u, 5
  %k = icmp eq i32 %n, 0
  %q = and i1 %lt5, %k
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %c = icmp ult i32 %u, 10
  %r = select i1 %c, i32 %v, i32 %a
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
