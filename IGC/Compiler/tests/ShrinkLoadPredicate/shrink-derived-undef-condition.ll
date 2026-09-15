;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; Shrinking copies the use condition into the load predicate, so the condition
; is read both by the load and by the select. A condition that may be undef or
; poison is frozen first, so that both reads agree.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %u may be undef, so %c may differ at every use. If the load reads %c as false
; and the select reads it as true, the result is the fallback value 13 instead of
; the loaded one. Freezing %c makes both reads agree.
; CHECK-LABEL: define i32 @derived_undef_condition(
; CHECK-NEXT:    %c = icmp eq i32 %u, 0
; CHECK-NEXT:    %c.fr = freeze i1 %c
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c.fr, i32 13)
; CHECK-NEXT:    %r = select i1 %c.fr, i32 %a, i32 0
; CHECK-NEXT:    ret i32 %r

define i32 @derived_undef_condition(ptr addrspace(1) %p, i32 %u) {
  %c = icmp eq i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 13)
  %r = select i1 %c, i32 %a, i32 0
  ret i32 %r
}

; %x is noundef, but `add nsw` may produce poison, so %c is frozen as well.
; CHECK-LABEL: define i32 @poison_condition(
; CHECK-NEXT:    %n = add nsw i32 %x, 1
; CHECK-NEXT:    %c = icmp eq i32 %n, 0
; CHECK-NEXT:    %c.fr = freeze i1 %c
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c.fr, i32 13)
; CHECK-NEXT:    %r = select i1 %c.fr, i32 %a, i32 0
; CHECK-NEXT:    ret i32 %r

define i32 @poison_condition(ptr addrspace(1) %p, i32 noundef %x) {
  %n = add nsw i32 %x, 1
  %c = icmp eq i32 %n, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 13)
  %r = select i1 %c, i32 %a, i32 0
  ret i32 %r
}

; %u is noundef, so %c is well defined: the load shrinks without a freeze.
; CHECK-LABEL: define i32 @noundef_condition(
; CHECK-NEXT:    %c = icmp eq i32 %u, 0
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %c
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %r = select i1 %c, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %r

define i32 @noundef_condition(ptr addrspace(1) %p, i1 noundef %S, i32 noundef %u, i32 %other) {
  %c = icmp eq i32 %u, 0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %r = select i1 %c, i32 %a, i32 %other
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
