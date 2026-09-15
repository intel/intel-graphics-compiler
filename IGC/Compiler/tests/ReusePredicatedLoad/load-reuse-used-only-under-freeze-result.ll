;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; Skipping the fallback select needs the use condition to imply the load
; predicate. The first check looks through the freezes this pass created. The
; proof after freezeShared() does not, so it is repeated even when nothing was
; frozen. Each case below passes the first check and then keeps the fallback
; select.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Nothing to freeze, and the repeated proof fails. %q may be undef through %t,
; so reusing %a freezes it. %c implies %q, but not %q.fr: %q.fr is one value
; chosen from what %q may be, and it is not known to be `or %n, %t`. The values
; %c and %q.fr share (%u) are well defined, so nothing is frozen, and the
; fallback select stays.
; CHECK-LABEL: define i32 @not_needed_proof_fails(
; CHECK:         %q.fr = freeze i1 %q
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %q.fr, i32 %a, i32 13
; CHECK-NEXT:    %c = icmp ne i32 %u, 0
; CHECK-NEXT:    %r = select i1 %c, i32 %[[SEL]], i32 0

define i32 @not_needed_proof_fails(ptr addrspace(1) %p, i32 noundef %u, i1 %t) {
  %n = icmp ne i32 %u, 0
  %q = or i1 %n, %t
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %c = icmp ne i32 %u, 0
  %r = select i1 %c, i32 %b, i32 0
  %s = add i32 %a, %r
  ret i32 %s
}

; Frozen, and the repeated proof fails. For %sum = add %x, %y, %m implies %q.
; The shared %sum and %x are frozen. The two freezes are chosen independently,
; so %sum.fr need not equal %x.fr + %y, and the `add` no longer relates the two
; comparisons. The proof fails, and both freezes are rolled back. Only the %q.fr
; from the reuse stays.
; CHECK-LABEL: define i32 @frozen_proof_fails(
; CHECK-NOT:     %sum.fr
; CHECK-NOT:     %x.fr
; CHECK:         %q.fr = freeze i1 %q
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %q.fr, i32 %a, i32 13
; CHECK-NEXT:    %m = icmp uge i32 %sum, %x
; CHECK-NEXT:    %r = select i1 %m, i32 %[[SEL]], i32 0

define i32 @frozen_proof_fails(ptr addrspace(1) %p, i32 %x, i32 %y) {
  %sum = add i32 %x, %y
  %q = icmp uge i32 %sum, %y
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %m = icmp uge i32 %sum, %x
  %r = select i1 %m, i32 %b, i32 0
  %s = add i32 %a, %r
  ret i32 %s
}

; The freeze fails. %c implies %q through the `and`, but the walk for shared
; values stops at the PHI %n, which may be undef. Nothing is frozen, and the
; fallback select stays.
; CHECK-LABEL: define i32 @freeze_fails(
; CHECK-NOT:     freeze
; CHECK:         %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %q, i32 %a, i32 13
; CHECK:         %r = select i1 %c, i32 %[[SEL]], i32 0

define i32 @freeze_fails(ptr addrspace(1) %p, i1 noundef %br, i32 noundef %u, i32 %x, i32 %y) {
entry:
  br i1 %br, label %left, label %right
left:
  br label %merge
right:
  br label %merge
merge:
  %n = phi i32 [ %x, %left ], [ %y, %right ]
  %q = icmp ult i32 %u, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %k = icmp eq i32 %n, 0
  %c = and i1 %q, %k
  %r = select i1 %c, i32 %b, i32 0
  %s = add i32 %a, %r
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
