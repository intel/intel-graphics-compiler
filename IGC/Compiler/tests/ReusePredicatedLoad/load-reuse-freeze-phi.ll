;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The walk for the values two predicates share does not look through PHIs. A
; PHI that may be undef, and is not itself frozen, may hide a shared value, so
; the reuse is rejected.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %m implies %q, and %q is shared. %m also reads %q through the PHI %t, which
; the walk stops at. %u may be undef, so %t may be too: no reuse, no freeze.
; CHECK-LABEL: define i32 @shared_through_phi(
; CHECK-NOT:     freeze
; CHECK:         %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
; CHECK-NEXT:    %s = add i32 %a, %b

define i32 @shared_through_phi(ptr addrspace(1) %p, i32 %u, i1 noundef %again) {
entry:
  %q = icmp ult i32 %u, 8
  br label %loop

loop:
  %t = phi i1 [ %q, %entry ], [ %q, %loop ]
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m = and i1 %q, %t
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
  %s = add i32 %a, %b
  br i1 %again, label %loop, label %exit

exit:
  ret i32 %s
}

; With a noundef %u the PHI cannot be undef or poison, and %b is reused.
; CHECK-LABEL: define i32 @noundef_through_phi(
; CHECK-NOT:     freeze
; CHECK:         %m = and i1 %q, %t
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %m, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[SEL]]

define i32 @noundef_through_phi(ptr addrspace(1) %p, i32 noundef %u, i1 noundef %again) {
entry:
  %q = icmp ult i32 %u, 8
  br label %loop

loop:
  %t = phi i1 [ %q, %entry ], [ %q, %loop ]
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m = and i1 %q, %t
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
  %s = add i32 %a, %b
  br i1 %again, label %loop, label %exit

exit:
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
