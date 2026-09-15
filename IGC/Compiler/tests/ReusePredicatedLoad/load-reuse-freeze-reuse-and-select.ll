;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The "load reuse" and the "removal of the fallback select" each freeze what their
; own proof needs. The second freeze only removes executions, so it cannot make the
; first proof false, and both freezes are kept.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %q2 implies %q1 through the shared %u, so %u is frozen for the reuse. %c
; implies %q2 through %q2 itself, which may still be undef through %v, so %q2
; is frozen too, and the fallback select is skipped.
; CHECK-LABEL: define i32 @reuse_and_skip_select(
; CHECK-NEXT:    %u.fr = freeze i32 %u
; CHECK-NEXT:    %q1 = icmp ult i32 %u.fr, 10
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
; CHECK-NEXT:    %qu = icmp ult i32 %u.fr, 5
; CHECK-NEXT:    %qv = icmp ult i32 %v, 5
; CHECK-NEXT:    %q2 = and i1 %qu, %qv
; CHECK-NEXT:    %q2.fr = freeze i1 %q2
; CHECK-NEXT:    %c = and i1 %q2.fr, %w
; CHECK-NEXT:    %r = select i1 %c, i32 %a, i32 0
; CHECK-NEXT:    %s = add i32 %a, %r
; CHECK-NEXT:    ret i32 %s

define i32 @reuse_and_skip_select(ptr addrspace(1) %p, i32 %u, i32 %v, i1 noundef %w) {
  %q1 = icmp ult i32 %u, 10
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
  %qu = icmp ult i32 %u, 5
  %qv = icmp ult i32 %v, 5
  %q2 = and i1 %qu, %qv
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 13)
  %c = and i1 %q2, %w
  %r = select i1 %c, i32 %b, i32 0
  %s = add i32 %a, %r
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
