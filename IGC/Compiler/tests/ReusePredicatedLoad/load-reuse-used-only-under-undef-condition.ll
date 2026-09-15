;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The fallback select is skipped when the use condition implies the load
; predicate. The condition is read by the select and the predicate by the load,
; so the implication holds only if both reads agree; a value that may be undef is
; frozen first.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The select condition is the predicate %q itself. Freezing %q makes the select
; and the load read the same value, and the fallback select is skipped.
; CHECK-LABEL: define i32 @used_only_under_if_true(
; CHECK-NEXT:    %q = icmp eq i32 %u, 0
; CHECK-NEXT:    %q.fr = freeze i1 %q
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT:    %r = select i1 %q.fr, i32 %a, i32 0
; CHECK-NEXT:    %s = add i32 %a, %r
; CHECK-NEXT:    ret i32 %s

define i32 @used_only_under_if_true(ptr addrspace(1) %p, i32 %u) {
  %q = icmp eq i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %r = select i1 %q, i32 %b, i32 0
  %s = add i32 %a, %r
  ret i32 %s
}

; %q is frozen when %a and %b are compared. !%c implies %q only through the
; shared %u, so %u is frozen too. Once %u is frozen, %q cannot be undef or
; poison, so %q.fr equals %q and !%c implies %q.fr. %b is observed only
; where %a read memory, and the fallback select is not needed.
; CHECK-LABEL: define i32 @used_only_under_if_false(
; CHECK-NEXT:    %u.fr = freeze i32 %u
; CHECK-NEXT:    %c = icmp eq i32 %u.fr, 0
; CHECK-NEXT:    %q = icmp ne i32 %u.fr, 0
; CHECK-NEXT:    %q.fr = freeze i1 %q
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT:    %r = select i1 %c, i32 0, i32 %a
; CHECK-NEXT:    %s = add i32 %a, %r
; CHECK-NEXT:    ret i32 %s

define i32 @used_only_under_if_false(ptr addrspace(1) %p, i32 %u) {
  %c = icmp eq i32 %u, 0
  %q = icmp ne i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %r = select i1 %c, i32 0, i32 %b
  %s = add i32 %a, %r
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
