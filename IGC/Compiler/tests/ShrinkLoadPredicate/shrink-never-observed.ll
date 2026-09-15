;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=1 \
; RUN:   --verify -S %s | FileCheck %s --check-prefix=BUDGET

; When the predicate proves the condition has the value the use does not want,
; the load is removed and its consumers take the fallback value directly.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The predicate is the condition, and the result is read only where it is false.
; CHECK-LABEL: define i32 @predicate_is_condition(
; CHECK-NEXT:    %result = select i1 %S, i32 %other, i32 0
; CHECK-NEXT:    ret i32 %result

define i32 @predicate_is_condition(ptr addrspace(1) %p, i1 noundef %S, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %result = select i1 %S, i32 %other, i32 %a
  ret i32 %result
}

; The predicate bounds %n below 5, the use needs it above 100: disjoint ranges.
; The comparison that fed only the predicate dies with the load.
; CHECK-LABEL: define i32 @disjoint_ranges(
; CHECK-NEXT:    %A = icmp ugt i32 %n, 100
; CHECK-NEXT:    %result = select i1 %A, i32 0, i32 %other
; CHECK-NEXT:    ret i32 %result

define i32 @disjoint_ranges(ptr addrspace(1) %p, i32 noundef %n, i32 %other) {
  %S = icmp ult i32 %n, 5
  %A = icmp ugt i32 %n, 100
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %result = select i1 %A, i32 %a, i32 %other
  ret i32 %result
}

; The fallback value is also the other incoming value, so the consumer folds away
; too.
; CHECK-LABEL: define i32 @consumer_folds(
; CHECK-NEXT:    ret i32 0

define i32 @consumer_folds(ptr addrspace(1) %p, i1 %S) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %result = select i1 %S, i32 0, i32 %a
  ret i32 %result
}

; A predicated load feeding another load's fallback operand is an operand of a call,
; which is not a value-forwarding use, so it is never shrunk. %b keeps its original
; and inherits %a's consumer.
; CHECK-LABEL: define i32 @fallback_operand_chain(
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %A, i32 0)
; CHECK-NEXT:    %r = select i1 %S, i32 %other, i32 %b
; CHECK-NEXT:    ret i32 %r

define i32 @fallback_operand_chain(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 noundef %S, i1 %A, i32 %other) {
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %A, i32 0)
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 %b)
  %r = select i1 %S, i32 %other, i32 %a
  ret i32 %r
}

; A removed load builds no mask, so it must not consume the mask budget:
; with a budget of one, the unobservable load is removed *and* the ordinary load
; that follows it still shrinks.
; CHECK-LABEL: define i32 @budget_not_charged(
; BUDGET-LABEL: define i32 @budget_not_charged(
; BUDGET-NEXT:    %predload.shrunk = and i1 %S, %A
; BUDGET-NEXT:    %live = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %predload.shrunk, i32 0)
; BUDGET-NEXT:    %rdead = select i1 %S, i32 %other, i32 0
; BUDGET-NEXT:    %rlive = select i1 %A, i32 %live, i32 %other
; BUDGET-NEXT:    %result = add i32 %rdead, %rlive
; BUDGET-NEXT:    ret i32 %result

define i32 @budget_not_charged(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 noundef %S, i1 noundef %A, i32 %other) {
  %dead = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %live = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %S, i32 0)
  %rdead = select i1 %S, i32 %other, i32 %dead
  %rlive = select i1 %A, i32 %live, i32 %other
  %result = add i32 %rdead, %rlive
  ret i32 %result
}

; The mirror of @budget_not_charged: here the load that spends the budget comes
; first, so a budget check on entry to the walk would stop before the unobservable
; load behind it. Removal is free, so %dead must still go at a budget of one.
; CHECK-LABEL: define i32 @budget_spent_before_removal(
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A
; CHECK-NEXT:    %live = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %rlive = select i1 %A, i32 %live, i32 %other
; CHECK-NEXT:    %rdead = select i1 %S, i32 %other, i32 0
; CHECK-NEXT:    %result = add i32 %rlive, %rdead
; CHECK-NEXT:    ret i32 %result
; BUDGET-LABEL: define i32 @budget_spent_before_removal(
; BUDGET-NEXT:    %predload.shrunk = and i1 %S, %A
; BUDGET-NEXT:    %live = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %predload.shrunk, i32 0)
; BUDGET-NEXT:    %rlive = select i1 %A, i32 %live, i32 %other
; BUDGET-NEXT:    %rdead = select i1 %S, i32 %other, i32 0
; BUDGET-NEXT:    %result = add i32 %rlive, %rdead
; BUDGET-NEXT:    ret i32 %result

define i32 @budget_spent_before_removal(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 noundef %S, i1 noundef %A, i32 %other) {
  %live = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %q, i64 4, i1 %S, i32 0)
  %dead = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %rlive = select i1 %A, i32 %live, i32 %other
  %rdead = select i1 %S, i32 %other, i32 %dead
  %result = add i32 %rlive, %rdead
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
