;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; A predicate that may evaluate to undef is frozen before a load is reused.
; The fallback select reads the predicate again, and without the freeze that read
; could disagree with the one at the reused load. Literal `i1 undef` predicates
; are covered by load-reuse-undef-predicate.ll.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %u may be undef, so %q may differ at every use. If %a reads %q as false and
; the fallback select reads it as true, %b becomes %a's fallback value 7,
; which %b can never return. Freezing %q makes both reads agree.
; CHECK-LABEL: define i32 @derived_undef_predicate(
; CHECK-NEXT:    %q = icmp eq i32 %u, 0
; CHECK-NEXT:    %q.fr = freeze i1 %q
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q.fr, i32 7)
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %q.fr, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[V1]]
; CHECK-NEXT:    ret i32 %s

define i32 @derived_undef_predicate(ptr addrspace(1) %p, i32 %u) {
  %q = icmp eq i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

; %u is noundef, so %q is well defined: the load is reused without a freeze.
; CHECK-LABEL: define i32 @noundef_predicate(
; CHECK-NEXT:    %q = icmp eq i32 %u, 0
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %q, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[V1]]
; CHECK-NEXT:    ret i32 %s

define i32 @noundef_predicate(ptr addrspace(1) %p, i32 noundef %u) {
  %q = icmp eq i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
