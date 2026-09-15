;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; Test where the freeze of a condition that may be undef is inserted, and that it
; is inserted once per value. Every use of the condition, including the ones the
; pass does not change, reads the frozen value.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; An argument is frozen at the start of the entry block, even when the load is
; in a later block. The mask follows its latest operand, the freeze.
; CHECK-LABEL: define i32 @argument_condition(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %cond.fr = freeze i1 %cond
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond.fr
; CHECK-NEXT:    %x = add i32 %other, 1
; CHECK-NEXT:    br label %body
; CHECK-EMPTY:
; CHECK-NEXT:  body:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %r = select i1 %cond.fr, i32 %a, i32 %x
; CHECK-NEXT:    ret i32 %r

define i32 @argument_condition(ptr addrspace(1) %p, i1 noundef %q, i1 %cond, i32 %other) {
entry:
  %x = add i32 %other, 1
  br label %body

body:
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %r = select i1 %cond, i32 %a, i32 %x
  ret i32 %r
}

; A PHI is frozen after the last PHI of its block, not directly after itself.
; CHECK-LABEL: define i32 @phi_condition(
; CHECK:       loop:
; CHECK-NEXT:    %cond = phi i1 [ %cond0, %entry ], [ %cond1, %loop ]
; CHECK-NEXT:    %fallback = phi i32 [ 0, %entry ], [ %r, %loop ]
; CHECK-NEXT:    %cond.fr = freeze i1 %cond
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %r = select i1 %cond.fr, i32 %a, i32 %fallback
; CHECK-NEXT:    br i1 %again, label %loop, label %exit

define i32 @phi_condition(ptr addrspace(1) %p, i1 noundef %q, i1 %cond0, i1 %cond1, i1 noundef %again) {
entry:
  br label %loop

loop:
  %cond = phi i1 [ %cond0, %entry ], [ %cond1, %loop ]
  %fallback = phi i32 [ 0, %entry ], [ %r, %loop ]
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %r = select i1 %cond, i32 %a, i32 %fallback
  br i1 %again, label %loop, label %exit

exit:
  ret i32 %r
}

; Two loads under the same condition share one freeze and one mask - MemOpt
; merges predicated loads only when their predicates are the same Value. The
; second load already sees the frozen condition, so it needs no freeze of its
; own. The use in %c, which the pass does not change, reads the frozen value too.
; CHECK-LABEL: define i32 @shared_freeze(
; CHECK-NEXT:    %cond = icmp ult i32 %n, 8
; CHECK-NEXT:    %cond.fr = freeze i1 %cond
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond.fr
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %sa = select i1 %cond.fr, i32 %a, i32 %other
; CHECK-NEXT:    %sb = select i1 %cond.fr, i32 %b, i32 %other
; CHECK-NEXT:    %c = zext i1 %cond.fr to i32
; CHECK-NEXT:    %r0 = add i32 %sa, %sb
; CHECK-NEXT:    %r = add i32 %r0, %c
; CHECK-NEXT:    ret i32 %r

define i32 @shared_freeze(ptr addrspace(1) %p, i1 noundef %q, i32 %n, i32 %other) {
  %cond = icmp ult i32 %n, 8
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %sa = select i1 %cond, i32 %a, i32 %other
  %sb = select i1 %cond, i32 %b, i32 %other
  %c = zext i1 %cond to i32
  %r0 = add i32 %sa, %sb
  %r = add i32 %r0, %c
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
