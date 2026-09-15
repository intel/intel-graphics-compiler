;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; A load used by two selects shrinks only when both select conditions are the
; same value. Two different instructions count as the same condition only when
; they provably compute the same value, not merely when they look identical.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %c1 and %c2 are the same comparison of the same operands, shrink.
;
; CHECK-LABEL: define i32 @equivalent_icmps(
; CHECK-NEXT:    %c1 = icmp ult i32 %n, 8
; CHECK-NEXT:    %c2 = icmp ult i32 %n, 8
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %c2
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %u1 = select i1 %c1, i32 %a, i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2, i32 %a, i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    ret i32 %r

define i32 @equivalent_icmps(ptr addrspace(1) %p, i1 %q, i32 noundef %n, i32 %x, i32 %y) {
  %c1 = icmp ult i32 %n, 8
  %c2 = icmp ult i32 %n, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %u1 = select i1 %c1, i32 %a, i32 %x
  %u2 = select i1 %c2, i32 %a, i32 %y
  %r = add i32 %u1, %u2
  ret i32 %r
}

; %c1 and %c2 load from the same address, but the store in between can change
; the value, so they are different conditions. The two selects disagree, no
; shrink.
; CHECK-LABEL: define i32 @conditions_across_store(
; CHECK-NEXT:    %c1 = load i1, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    store i1 false, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    %c2 = load i1, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %u1 = select i1 %c1, i32 %a, i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2, i32 %a, i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    ret i32 %r

define i32 @conditions_across_store(ptr addrspace(1) %p, ptr addrspace(1) %g, i1 %q, i32 %x, i32 %y) {
  %c1 = load i1, ptr addrspace(1) %g, align 1
  store i1 false, ptr addrspace(1) %g, align 1
  %c2 = load i1, ptr addrspace(1) %g, align 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %u1 = select i1 %c1, i32 %a, i32 %x
  %u2 = select i1 %c2, i32 %a, i32 %y
  %r = add i32 %u1, %u2
  ret i32 %r
}

; Each `freeze i1 poison` picks its own arbitrary value, so %g1 and %g2 may
; differ even though the instructions are identical. No shrink.
; CHECK-LABEL: define { i1, i1, i32, i32 } @independent_freezes(
; CHECK-NEXT:    %g1 = freeze i1 poison
; CHECK-NEXT:    %g2 = freeze i1 poison
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %s1 = select i1 %g1, i32 %a, i32 0
; CHECK-NEXT:    %s2 = select i1 %g2, i32 %a, i32 0
; CHECK-NEXT:    %v0 = insertvalue { i1, i1, i32, i32 } poison, i1 %g1, 0
; CHECK-NEXT:    %v1 = insertvalue { i1, i1, i32, i32 } %v0, i1 %g2, 1
; CHECK-NEXT:    %v2 = insertvalue { i1, i1, i32, i32 } %v1, i32 %s1, 2
; CHECK-NEXT:    %v3 = insertvalue { i1, i1, i32, i32 } %v2, i32 %s2, 3
; CHECK-NEXT:    ret { i1, i1, i32, i32 } %v3

define { i1, i1, i32, i32 } @independent_freezes(ptr addrspace(1) %p, i1 %q) {
  %g1 = freeze i1 poison
  %g2 = freeze i1 poison
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %s1 = select i1 %g1, i32 %a, i32 0
  %s2 = select i1 %g2, i32 %a, i32 0
  %v0 = insertvalue { i1, i1, i32, i32 } poison, i1 %g1, 0
  %v1 = insertvalue { i1, i1, i32, i32 } %v0, i1 %g2, 1
  %v2 = insertvalue { i1, i1, i32, i32 } %v1, i32 %s1, 2
  %v3 = insertvalue { i1, i1, i32, i32 } %v2, i32 %s2, 3
  ret { i1, i1, i32, i32 } %v3
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
