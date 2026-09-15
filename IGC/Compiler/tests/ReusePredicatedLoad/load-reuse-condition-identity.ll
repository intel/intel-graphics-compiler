;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; Two structurally identical conditions need not compute the same value: the
; condition loads below are separated by a store. The use predicates therefore
; disagree, the later load counts as observable everywhere, and its fallback
; value survives in a select. A loaded condition may be undef, so the predicate
; %c1 is frozen before it is reused.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @identical_condition_loads(
; CHECK-NEXT:    %c1 = load i1, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    %c1.fr = freeze i1 %c1
; CHECK-NEXT:    store i1 false, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    %c2 = load i1, ptr addrspace(1) %g, align 1
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c1.fr, i32 0)
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %c1.fr, i32 %a, i32 7
; CHECK-NEXT:    %u1 = select i1 %c1.fr, i32 %[[V1]], i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2, i32 %[[V1]], i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    %r2 = add i32 %r, %a
; CHECK-NEXT:    ret i32 %r2

define i32 @identical_condition_loads(ptr addrspace(1) %p, ptr addrspace(1) %g, i32 %x, i32 %y) {
  %c1 = load i1, ptr addrspace(1) %g, align 1
  store i1 false, ptr addrspace(1) %g, align 1
  %c2 = load i1, ptr addrspace(1) %g, align 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c1, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c1, i32 7)
  %u1 = select i1 %c1, i32 %b, i32 %x
  %u2 = select i1 %c2, i32 %b, i32 %y
  %r = add i32 %u1, %u2
  %r2 = add i32 %r, %a
  ret i32 %r2
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
